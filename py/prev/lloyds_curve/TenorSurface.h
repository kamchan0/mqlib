#ifndef __LIBRARY_PRICERS_FLEXYCF_TENORSURFACE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TENORSURFACE_H_INCLUDED
#pragma once

//	FlexYCF
#include "KnotPoint.h"
#include "Dictionary.h"
#include "Gradient.h"
#include "CurveType.h"

//	LTQC
#include "VectorDouble.h"
#include "Macros.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( Problem )
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( CurveType )
    FWD_DECLARE_SMART_PTRS( InterpolationMethod )
    FWD_DECLARE_SMART_PTRS( CurveFormulation )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )
	FWD_DECLARE_SMART_PTRS( TenorSurfaceInterpolation )

	class IKnotPointFunctor;
    class CloneLookup;

	typedef std::vector<std::vector<std::pair<double, double>>> knot_points_container; 

    class TenorSurface {
	public:
		typedef Dictionary<CurveTypeConstPtr, CurveFormulationPtr, CurveType::DereferenceLess>    TypedCurves;
		typedef Dictionary<double, CurveFormulationPtr>											BucketedCurves;
	private: 
		
        typedef std::vector<CurveTypeConstPtr>              CurveTypeContainer;

    public:
      
		explicit TenorSurface(const LTQuant::GenericData& masterTable);
        TenorSurface(TenorSurface const& original, CloneLookup& lookup);

        typedef TypedCurves::const_iterator const_iterator;
        
        const_iterator begin() const;
        const_iterator end() const;

        std::vector<double> getFlowTimes() const;
  
        bool curveExists(const CurveTypeConstPtr& tenor) const;
        double interpolateCurve(const double tenor, const double flowTime) const;
      
     /*   void accumulateCurveGradient(const CurveTypeConstPtr tenor, 
                                     const double flowTime,
                                     double multiplier,
                                     GradientIterator gradientBegin,
                                     GradientIterator gradientEnd,
									 const CurveTypeConstPtr& curveType = CurveType::AllTenors()) const;
        
        void accumulateCurveGradient(const double tenor, 
                                     const double flowTime,
                                     double multiplier,
                                     GradientIterator gradientBegin,
                                     GradientIterator gradientEnd,
									 const CurveTypeConstPtr& curveType = CurveType::AllTenors()) const;
*/
        void accumulateGradient(const double tenor, 
                                const double flowTime,
                                double multiplier,
                                GradientIterator gradientBegin,
                                GradientIterator gradientEnd) const;

        /// Returns the number of unknowns on all curves of the tenor spread surface
        size_t getNumberOfUnknowns() const;
		
		/// Returns the number of unknown on the spread curve of the specified curve type
		size_t getNumberOfUnknowns(const CurveTypeConstPtr& tenor) const;

        void addKnotPoint(const CurveTypeConstPtr tenor, const KnotPoint& knotPoint);
        void update();
        void finalize();

        /// Adds the unknowns of the tenor spread surface to the problem
        void addUnknownsToProblem(const LTQuant::ProblemPtr& problem);

		void addUnknownsToProblem(const LTQuant::ProblemPtr& problem, IKnotPointFunctor& onKnotPointVariableAddedToProblem, const CurveTypeConstPtr& curveType = CurveType::AllTenors());
		void updateVariablesFromShifts(const LTQC::VectorDouble::const_iterator shiftsBegin, const LTQC::VectorDouble::const_iterator shiftsEnd);
        
		void initializeKnotPoints(const CurveTypeConstPtr& tenor);
        void fillWithIndexSpineCurvesData(const LTQuant::GenericDataPtr& spineCurvesData) const;
		void getCurveInternalData(knot_points_container& kpc) const;
		void assignCurveInternalData(knot_points_container::const_iterator it);

        //Adds the unfixed knot points of the spine curve
        void getIndexSpineCurvesUnfixedKnotPoints(std::list<double>& points) const;
        void createTenorCurves(const LTQuant::GenericDataPtr curvesInterpolationTable, const LeastSquaresResidualsPtr leastSquaresResiduals);

	private:
		size_t getNumberOfUnknownsUpToTenor(const double tenor) const;
        void checkIsTenor(const CurveTypeConstPtr curveType, const std::string& checkedFunctionName) const;
       
      
        TenorSurface(TenorSurface const&); // deliberately disabled as won't clone properly

        // Nested comparator struct to ease interpolation between tenor curves
        struct YearFractionKeyValueCompare
        {
            typedef FlexYCF::TenorSurface::TypedCurves::KeyValuePair KeyValuePair;
            
            bool operator()(const KeyValuePair& lhs, const KeyValuePair& rhs) const;
            bool operator()(const KeyValuePair& lhs, const double rhs) const;
            bool operator()(const double lhs, const KeyValuePair& rhs) const;

        private:
            bool operator()(const double lhs, const double rhs) const;
        };

        TypedCurves				     m_curves;
		TenorSurfaceInterpolationPtr m_tsInterpolation;
        size_t					     m_totalNumberOfUnknownKnotPoints;    
        std::vector<double>		     m_flowTimes;
    };  
    DECLARE_SMART_PTRS( TenorSurface )
}   //  FlexYCF
#endif 