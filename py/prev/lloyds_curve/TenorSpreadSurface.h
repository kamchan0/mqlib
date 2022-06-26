/*****************************************************************************

    TenorSpreadSurface

	Represents the surface supported by several index curves
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TENORSPREADSURFACE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TENORSPREADSURFACE_H_INCLUDED
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
    FWD_DECLARE_SMART_PTRS( ICurve )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )
	LTQC_FWD_DECLARE_SMART_PTRS( BaseTssInterpolation )
	
	class IKnotPointFunctor;
    class CloneLookup;
	typedef std::vector<std::vector<std::pair<double, double>>> knot_points_container;

	double multiTenorInitializationFunc(const double spotRate, const double x);


    /// TenorSpreadSurface encapsulates the tenor-typed spread curves 
    /// over the base rate curve in a MultiTenorModel.
    ///
    /// Note: if the base rate is a tenor, the corresponding spread curve
    /// in the surface is initialized with a fixed (0,0) point and flat
    /// interpolation to enforce a flat spread curve.
    /// An error is thrown if another point is attempted to be added on 
    /// this spread curve.
    /// 
    /// Remark: TenorSpreadSurface interface has a lot in common with the TypedCurve's one
    class TenorSpreadSurface
    {
	public:
		typedef Dictionary<CurveTypeConstPtr, ICurvePtr, CurveType::DereferenceLess>    TypedCurves;
    
	private: 
		typedef Dictionary<double, ICurvePtr>		BucketedCurves;
        typedef std::vector<CurveTypeConstPtr>      CurveTypeContainer;

    public:
        explicit TenorSpreadSurface(const CurveTypeConstPtr& baseRate);
		explicit TenorSpreadSurface(const CurveTypeConstPtr& baseRate,
									const LTQuant::GenericData& masterTable);
        TenorSpreadSurface(TenorSpreadSurface const& original, CloneLookup& lookup);

        typedef TypedCurves::const_iterator const_iterator;
        
        const_iterator begin() const;
        const_iterator end() const;

        std::vector<double> getFlowTimes() const;
  
        /// Returns whether the curve exists
        bool curveExists(const CurveTypeConstPtr& tenor) const;
        
        /// Interpolates along an existing tenor
		//	SOON: OBSOLETE
        double interpolateCurve(const double tenor, const double flowTime) const;

        /// Interpolates between curves
		//	SOON: delegate to the inner tenor/time surface interpolator
        double interpolate(const double tenor, const double flowTime) const;

		// compute the gradient of the curve whose tenor specified, at time flowTime, relative to ALL
        //      unknown knot-points on the surface
        // Consequently, the gradient partial derivatives will be all equal to 0, except 4 at most 
        //  (depending on the interpolation method for this tenor curve.
        // Note: this calculates the gradient on a given curve
        void accumulateCurveGradient(const CurveTypeConstPtr tenor, 
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

		//void addUnknownsToProblem(const LTQuant::ProblemPtr& problem,
		//						  IKnotPointFunctor& onKnotPointVariableAddedToProblem);

		/// Add the unknowns on the tenor spread surface with the specified 
		///	curve type (by default, add them all) to the problem
		/// calling the specified functor on each addition of a knot-point
		///	related variable
		void addUnknownsToProblem(const LTQuant::ProblemPtr& problem, 
								  IKnotPointFunctor& onKnotPointVariableAddedToProblem,
								  const CurveTypeConstPtr& curveType = CurveType::AllTenors());
		void updateVariablesFromShifts(const LTQC::VectorDouble::const_iterator shiftsBegin,
									  const LTQC::VectorDouble::const_iterator shiftsEnd);
        
	

		// initializes the 
		void initializeKnotPoints(const CurveTypeConstPtr& tenor,
			 					  const double initialSpotRate);

		// Fill the specified generic data with the spine curves data of the tenor spread surface
        void fillWithIndexSpineCurvesData(const LTQuant::GenericDataPtr& spineCurvesData) const;
        
        //Adds the unfixed knot points of the spine curve
        void getUnfixedIndexSpineCurvesKnotPoints(std::list<double>& points) const;

        virtual std::ostream& print(std::ostream& out) const;

        void createTenorSpreadCurves(const LTQuant::GenericDataPtr curvesInterpolationTable,
                                     const LeastSquaresResidualsPtr leastSquaresResiduals);

		void getCurveInternalData(knot_points_container& kpc) const;

		void assignCurveInternalData(knot_points_container::const_iterator it);

    private:
		//	void buildBucketedCurves();
		size_t getNumberOfUnknownsUpToTenor(const double tenor) const;

        void checkIsTenor(const CurveTypeConstPtr curveType,
                          const std::string& checkedFunctionName) const;

        void checkIsNotBaseRate(const CurveTypeConstPtr tenor,
                                const std::string& checkedFunctionName,
                                const std::string& errorMsg) const;

        // void initCurveCacheInfo(const LTQuant::GenericDataPtr /*cacheParametersTable*/ data);
        bool useCurveCache(const CurveTypeConstPtr tenor) const;
        TenorSpreadSurface(TenorSpreadSurface const&); // deliberately disabled as won't clone properly

        // Nested comparator struct to ease interpolation between tenor curves
        struct YearFractionKeyValueCompare
        {
            typedef FlexYCF::TenorSpreadSurface::TypedCurves::KeyValuePair KeyValuePair;
            
            bool operator()(const KeyValuePair& lhs, const KeyValuePair& rhs) const;
            bool operator()(const KeyValuePair& lhs, const double rhs) const;
            bool operator()(const double lhs, const KeyValuePair& rhs) const;

        private:
            bool operator()(const double lhs, const double rhs) const;
        };

        CurveTypeConstPtr		m_baseRate;
        TypedCurves				m_spreadCurves;
		BaseTssInterpolationPtr m_tssInterpolation;
        size_t					m_totalNumberOfUnknownKnotPoints;    // recomputed at each finalize
        std::vector<double>		m_flowTimes;
        std::vector<double>		m_smallTenorCurveGradient;

        CurveTypeContainer		m_cachedCurveTypes;
    };  //  SpreadSurface

    DECLARE_SMART_PTRS( TenorSpreadSurface )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const TenorSpreadSurface& tenorSpreadSurface)
		{
			return tenorSpreadSurface.print(out);
		}
	}
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_TENORSPREADSURFACE_H_INCLUDED