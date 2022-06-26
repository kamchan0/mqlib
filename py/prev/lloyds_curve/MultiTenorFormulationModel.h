#ifndef __LIBRARY_PRICERS_FLEXYCF_MULTITENORFORMULATIONMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MULTITENORFORMULATIONMODEL_H_INCLUDED
#pragma once

//	FlexYCF
#include "MultiCurveModel.h"
#include "TenorSpreadSurface.h"
#include "TenorSurface.h"
#include "Maths\Problem.h"
#include "CurveType.h"
#include "Dictionary.h"
#include "Gradient.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( InterpolationMethod )
	FWD_DECLARE_SMART_PTRS( CurveFormulation )
    
    class MultiTenorFormulationModel : public MultiCurveModel
    {    
    private:
        MultiTenorFormulationModel(const LT::date valueDate, const LTQuant::GenericDataPtr& masterTable, const LTQuant::FlexYCFZeroCurvePtr parent);

    public:
        
        static std::string getName() { return "MultiTenorFormulation"; }
        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent);
       
        virtual double getDiscountFactor(const double flowTime) const;
        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const;

        virtual void accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;

        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor, 
                                                           double multiplier, 
														   GradientIterator gradientBegin, 
                                                           GradientIterator gradientEnd) const;
        
		virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const;

        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor, 
                                                           double multiplier, 
														   GradientIterator gradientBegin, 
                                                           GradientIterator gradientEnd,
														   const CurveTypeConstPtr& curveType) const;

        void addKnotPoint(const CurveTypeConstPtr curveTypePtr, const KnotPoint& knotPoint);
        
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem);
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem,
									   	   IKnotPointFunctor& onKnotPointVariableAddedToproblem);
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem,
										   const CurveTypeConstPtr& curveType,
										   IKnotPointFunctor& onKnotPointVariableAddedToProblem);
		virtual void updateVariablesFromShifts(const LTQC::VectorDouble& variableShifts);
		virtual void update();
        virtual void finalize();

		virtual void initializeKnotPoints();
		virtual void initializeKnotPoints(const CurveTypeConstPtr& curveType,
										  const double initialSpotRate);

		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
														  const double discountFactor,
														  const CurveTypeConstPtr& curveType) const;

		virtual size_t getNumberOfUnknowns(const CurveTypeConstPtr& curveType) const;

		virtual LTQuant::GenericDataPtr getSpineCurvesDetails() const;

        // Adds the unfixed spine curves knot points to the list
        virtual void getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const;

		CurveTypeConstPtr getBaseRate() const;
       

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

        virtual void finishCalibration();

		const LTQC::Matrix& getFullJacobian() const;

		LT::TablePtr getFullJacobian( const bool includeHeadings ) const;
       
		void getSpineInternalData(SpineDataCachePtr& sdp) const;
		void assignSpineInternalData(SpineDataCachePtr& sdp);
    
	protected:
        virtual void onKnotPointsPlaced();

        MultiTenorFormulationModel(MultiTenorFormulationModel const& original, CloneLookup& lookup);

    private:
        void throwError(const string& functionName, const string& errorMsg) const;
        MultiTenorFormulationModel(MultiTenorFormulationModel const&); // deliberately disabled as won't clone properly

        CurveFormulationPtr m_discountCurve;
		TenorSurface		m_tenorSurface;
        size_t				m_numberOfBaseRateCurveUnknowns; 
        size_t				m_totalNumberOfUnknowns;         

		 mutable LTQC::Matrix	m_fullJacobian;

		 void findDependentModel(BaseModelConstPtr&   dependentModel) const;
    };  

    DECLARE_SMART_PTRS( MultiTenorFormulationModel )
}   // FlexYCF
#endif 