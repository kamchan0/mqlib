/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BASECURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BASECURVE_H_INCLUDED
#pragma once

#include "ICurve.h"
#include "KnotPoint.h"
#include "KnotPoints.h"
#include "BaseExtrapolationMethods.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( InterpolationCurve )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )

    /// Represents a curve that has a collection of knot-points,
    /// an InterpolationCurve, a LeftExtrapolation and a 
    /// RightExtrapolation.
    /// 
    /// Notes: 
    /// - InterpolationCurve knows what the unknown variables 
    /// of the curve are.
    /// - KnotPoints are shared by BaseCurve and 
    /// InterpolationCurve base class.
    class BaseCurve: public ICurve
    {
    public:
        BaseCurve();
        
        BaseCurve(const KnotPointsPtr knotPoints,
                  const InterpolationCurvePtr interpolationCurve,
                  const LeftExtrapolationPtr leftExtrapolationMethod,
                  const RightExtrapolationPtr rightExtrapolationMethod);
        
        static std::string getName();
        
        static ICurvePtr createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable,
                                        const LeastSquaresResidualsPtr leastSquaresResiduals);

        static ICurvePtr create(const LTQuant::GenericDataPtr interpolationDetailsTable);

        /// Evaluates the value of the curve function at x
        virtual double evaluate(const double x) const;

        virtual void accumulateGradient(const double x, 
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd) const;

        virtual void addKnotPoint(const KnotPoint& knotPoint);
        
        /// Note : to be called AFTER finalize
        virtual void addUnknownsToProblem(const LTQuant::ProblemPtr& problem);
		virtual void addUnknownsToProblem(const LTQuant::ProblemPtr& problem,
										  IKnotPointFunctor& onKnotPointVariableAddedToProblem);
		virtual void updateVariablesFromShifts(const LTQC::VectorDouble::const_iterator shiftsBegin,
											  const LTQC::VectorDouble::const_iterator shiftsEnd);
        virtual void finalize();

        /// Note : to be called AFTER finalize
        virtual void update();

        virtual size_t getNumberOfUnknowns() const;
		virtual double getUnknown(const size_t index) const;
		virtual void setUnknown(const size_t index, const double value);
		virtual size_t getNumberOfKnots() const;
		virtual LTQuant::GenericDataPtr getCurveDetails() const;
		virtual void getCurveInternalData(knot_points_container& kpc) const;
		virtual void assignCurveInternalData(knot_points_container::const_iterator it);
        virtual void getUnfixedKnotPoints(std::list<double>& points) const;
        virtual std::ostream& print(std::ostream& out) const;

        /// Returns a const_iterator point to the first knot-point
        KnotPoints::const_iterator begin() const;

        /// Returns an const_iterator pointing to one past the last knot-point
        KnotPoints::const_iterator end() const;

        /// Initializes each knot-point (x,y) value with
        /// the initializing function f by setting y = f(x)
        /// Note: only used by CurveFormulation's
		/// initializateKnotPoints should become obsolete and be replaced with initialize function below
		//  void initializeKnotPoints(const KnotPoints::InitFunction& initFunction);

		// To replace initializeKnotPoints
		virtual void initialize(const CurveInitializationFunction& curveInitializationFunction);

		/// Delegates what to do when knot-points are initialized
		/// to its interpolation curve
		void onKnotPointsInitialized() const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        BaseCurve(BaseCurve const& original, CloneLookup& lookup);
        void setInterpolationCurve(const InterpolationCurvePtr interpolationCurve);
        void setLeftExtrapolationMethod(const LeftExtrapolationPtr leftExtrapolationMethod);
        void setRightExtrapolationMethod(const RightExtrapolationPtr rightExtrapolationMethod);
    
    private:
        BaseCurve(BaseCurve const&); // deliberately disabled as won't clone properly
		friend void setSeparationPoint(const LT::date, const BaseCurve&);

        KnotPointsPtr           m_knotPoints;
        InterpolationCurvePtr   m_interpolationCurve;
        LeftExtrapolationPtr    m_leftExtrapolationMethod;
        RightExtrapolationPtr   m_rightExtrapolationMethod;
    };

    DECLARE_SMART_PTRS( BaseCurve )
}
#endif //__LIBRARY_PRICERS_FLEXYCF_NEWBASECURVE_H_INCLUDED