#ifndef __LIBRARY_PRICERS_FLEXYCF_MULTITENOROISFUNDINGMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MULTITENOROISFUNDINGMODEL_H_INCLUDED
#pragma once

//	FlexYCF
#include "MultiCurveModel.h"
#include "TenorSpreadSurface.h"
#include "Maths\Problem.h"
#include "CurveType.h"
#include "Dictionary.h"
#include "Gradient.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( InterpolationMethod )
    FWD_DECLARE_SMART_PTRS( ICurve )

    /// MultiTenorOISFundingModel is a multi-curve model that represents
    /// curves as spreads over a base curve, discount curve is a spread to 1d curve.
    /// Curves can be of a specified Tenor or the discount curve.
    /// The spread for the base curve is 0.
    ///
    /// Notes:
    /// - this model uses the logFvf formulation
    /// - for gradient calculations, the unknowns are arbitrarily 
    /// sorted in this order: the base rate curve unknowns, then the 
    /// discount spread ones, and the Tenor  spread surface unknowns
    class MultiTenorOISFundingModel : public MultiCurveModel
    {    
    private:
        MultiTenorOISFundingModel(const LT::date valueDate,
                        const CurveTypeConstPtr& baseRate,
						const LTQuant::GenericDataPtr& masterTable,
                        const LTQuant::FlexYCFZeroCurvePtr parent);

    public:
        /// Returns the name of the type of model as used by the ModelFactory
        static std::string getName()
        {
            return "MultiTenorOISFunding";
        }

        /// Creates a MultiTenorOISFundingModel from a table
        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent);
        
        /// Returns the type of the Base Rate.
        virtual CurveTypeConstPtr getBaseRate() const;
       
        virtual double getDiscountFactor(const double flowTime) const;

        /// Note: Enforces interpolation along the nearest curve on the Tenor Spread Surface.
        /// It does not interpolate accross tenors for now
        virtual double getTenorDiscountFactor(const double flowTime, 
                                              const double tenor) const;

        virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd) const;

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



        /// Add a knot-point to the specified curve of the model. If base rate and the curve type
        ///  are Discount, the knot-point should be added to the base rate and the discount spread 
        /// curve should be 0 and flat.
        /// Shoud it belong to the MultiCurveModel interface (replacing CurveTypeConstPtr with double for the Tenor)?
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

		// see also: InflationModel::inflationIndexToVariable
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
														  const double discountFactor,
														  const CurveTypeConstPtr& curveType) const;

		virtual size_t getNumberOfUnknowns(const CurveTypeConstPtr& curveType) const;

        virtual LTQuant::GenericDataPtr getSpineCurvesDetails() const;
        
        // Adds the unfixed spine curve knot points to the list
        virtual void getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const;

        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

        /// after calib is finished destory all of the calibration
        /// instruments so they do not take up memory via the CachedInstrument component
        virtual void finishCalibration();

		void getSpineInternalData(SpineDataCachePtr& sdp) const;
		void assignSpineInternalData(SpineDataCachePtr& sdp);

    protected:
        virtual void onKnotPointsPlaced();

        MultiTenorOISFundingModel(MultiTenorOISFundingModel const& original, CloneLookup& lookup);

    private:
        void throwError(const string& functionName, const string& errorMsg) const;
        MultiTenorOISFundingModel(MultiTenorOISFundingModel const&); // deliberately disabled as won't clone properly

        CurveTypeConstPtr   m_baseRate;
        CurveTypeConstPtr   m_overnightRate;
        const double        m_overnightTenor;

        ICurvePtr           m_baseRateCurve;
        ICurvePtr           m_discountSpreadCurve;
        TenorSpreadSurface  m_tenorSpreadSurface;

        size_t m_numberOfBaseRateCurveUnknowns; //  number of unknowns on the base rate curve
        size_t m_totalNumberOfUnknowns;         //  number of unknowns on all curves and the spread surface
    };  //  MultiTenorOISFundingModel

    DECLARE_SMART_PTRS( MultiTenorOISFundingModel )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const MultiTenorOISFundingModelPtr multiTenorModel)
		{
			return multiTenorModel->print(out);
		}
	}
}   // FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_MULTITENOROISFUNDINGMODEL_H_INCLUDED