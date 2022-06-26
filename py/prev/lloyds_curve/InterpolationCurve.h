/*****************************************************************************

    InterpolationCurve

	Base class for "curves" that interpolate (no extrapolation).
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONCURVE_H_INCLUDED
#pragma once

#include "CurveAlgorithm.h"
#include "KnotPoint.h"
#include "KnotPoints.h"
#include "KnotPointFunctor.h"
#include "Gradient.h"
#include "ICloneLookup.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( Problem );
}


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( KnotPoints )

    /// A base class for concrete interpolation classes
    class InterpolationCurve : public ICloneLookup	//: ICurveAlgorithm
    {
    public:
        /// Interpolates the curve at point x
        virtual double interpolate(const double x) const = 0; 

        /// Accumulates to the gradient at x from gradientBegin to gradientEnd
        /// using the specified multiplier.
        virtual void accumulateGradient(const double x, 
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd) const = 0;

        /// Adds unknowns to the specified problem
		/// No longer pure virtual

		void addUnknownsToProblem(const LTQuant::ProblemPtr& problem,
								  IKnotPointFunctor const& onKnotPointVariableAddedToProblem = doNothingKpFunctor());
       
        /// Finalizes the curve
        virtual void finalize() = 0;

        /// Updates the curve
        virtual void update() = 0;

        /// Returns the number of unknowns
        virtual size_t getNumberOfUnknowns() const = 0;

		///	Returns the index-th unknown
		virtual double getUnknown(const size_t index) const = 0;

		///	Sets the index-th unknown to the specified value
		virtual void setUnknown(const size_t index, const double value) = 0;

        /// Adds the specified knot-point to the curve
        void addKnotPoint(const KnotPoint& knotPoint);          // and/or addKnot(..)

        /// Optional further logic when a knot-point is added
        virtual void onKnotPointAdded(const KnotPoint& knotPoint);

        /// Returns whether the curve is empty
        bool empty() const;

        /// Returns the value of the first knot
        double xMin() const;

        /// Returns the value of the last knot
        double xMax() const;
        
        /// Returns the number of knot-points
        size_t size() const;    // getNumberOfKnotPoints would be more explicit
        
        /// Returns a const reference to the knot-point at the speficied index
        const KnotPoint& getKnotPoint(const size_t index) const;

        /// Returns a reference to the knot-point at the speficied index
        KnotPoint& getKnotPoint(const size_t index);
        
        typedef KnotPoints::const_iterator const_iterator;
        
        /// Returns an iterator pointing to the first knot-point
        KnotPoints::const_iterator begin() const;

        /// Returns an iterator pointing to one past last knot-point
        KnotPoints::const_iterator end() const;

        /// Prints the InterpolationCurve
        virtual std::ostream& print(std::ostream& out) const = 0;


		virtual void addKnotPointVariableToProblem(const size_t kpIndex,
												   const LTQuant::ProblemPtr& problem) = 0;
	
		virtual void onKnotPointsInitialized()
		{
			// do nothing by default
		}

	protected:
        InterpolationCurve();
        InterpolationCurve(InterpolationCurve const& original, CloneLookup& lookup);

        void setKnotPoints(const KnotPointsPtr knotPoints);
        
        void addUnknownKnotPointsYsToProblemInRange(const LTQuant::ProblemPtr& problem,
                                                    const double min,
                                                    const double max);
       
        void setYsFromXsWithFunction(const KnotPoints::InitFunction& func);
 
        KnotPoints::const_iterator lowerBound(const double x) const;
        KnotPoints::const_iterator upperBound(const double x) const;

        size_t getNumberOfUnknownKnotPoints() const;

    private:
        InterpolationCurve(InterpolationCurve const&); // deliberately disabled as won't clone properly

        KnotPointsPtr m_knotPoints;
    };  //  InterpolationCurve

    DECLARE_SMART_PTRS( InterpolationCurve )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const InterpolationCurve& interpolationCurve)
		{
			return interpolationCurve.print(out);
		}
	}
}
#endif //__LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONCURVE_H_INCLUDED