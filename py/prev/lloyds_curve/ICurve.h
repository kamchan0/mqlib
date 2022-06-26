/*****************************************************************************

	ICurve

	Represents the interface for all FlexYCF curves.
    

    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_ICURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_ICURVE_H_INCLUDED
#pragma once

#include <iterator>
//	FlexYCF
#include "Gradient.h"
#include "KnotPoint.h"
#include "KnotPoints.h"
#include "KnotPointFunctor.h"
#include "CurveInitializationFunction.h"
#include "ICloneLookup.h"

//	LTQC
#include "VectorDouble.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( Problem )
}



namespace FlexYCF
{
	typedef std::vector<std::vector<std::pair<double, double>>> knot_points_container; 
    /// Defines the interface for any mathematical curve
    /// within FlexYCF
    class ICurve : public ICloneLookup
    {
    public:
        virtual ~ICurve()
        {
        }

        /// Returns a const_iterator point to the first knot-point
        virtual KnotPoints::const_iterator begin() const=0;

        /// Returns an const_iterator pointing to one past the last knot-point
        virtual KnotPoints::const_iterator end() const=0;

         /// Evaluates the value of the curve function at x
        virtual double evaluate(const double x) const = 0;

        /// Accumulates to the gradient at x from gradientBegin to gradientEnd
        /// using the specified multiplier.
        virtual void accumulateGradient(const double x, 
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd) const = 0;
        
        // Not clear to keep this one... ongoing discussion about adding knot-point
        // or knots + initialize function
        virtual void addKnotPoint(const KnotPoint& knotPoint) = 0;
    
        /// Adds the unknowns of the curve to the specified problem
        virtual void addUnknownsToProblem(const LTQuant::ProblemPtr& problem) = 0;
		
		/// Adds the unknowns of the curve to the specified problem, letting
		/// derived implementations to support a call-back
		virtual void addUnknownsToProblem(const LTQuant::ProblemPtr& problem,
										  IKnotPointFunctor& /* onKnotPointVariableAddedToProblem */)
		{
			addUnknownsToProblem(problem);
		}

		virtual void updateVariablesFromShifts(const LTQC::VectorDouble::const_iterator shiftsBegin,
											  const LTQC::VectorDouble::const_iterator shiftsEnd) = 0;

        
        /// Finalizes the curve 
        /// Usage note: should be called once all knot-points have been 
        /// added to the curve
        virtual void finalize() = 0;

		/// Initializes the curve
		virtual void initialize(const CurveInitializationFunction& curveInitializationFunction) = 0;
        
		/// Updates the curve
        virtual void update() = 0;
        
        /// Returns the number of unknowns of the curve
        virtual size_t getNumberOfUnknowns() const = 0;
    
		///	Returns the index-th unknown
		virtual double getUnknown(const size_t index) const = 0;

		///	Sets the index-th unknown to the specified value
		virtual void setUnknown(const size_t index, const double value) = 0;

		/// Shifts the index-th unknown by the specified value
		inline void shiftUnknown(const size_t index, const double shift)
		{
			setUnknown(index, getUnknown(index) + shift);
		}

		/// Returns the number of knots
		virtual size_t getNumberOfKnots() const = 0;

		/// Returns the curve details
		virtual LTQuant::GenericDataPtr getCurveDetails() const = 0;

		virtual void getCurveInternalData(knot_points_container& kpc) const = 0;

		virtual void assignCurveInternalData(knot_points_container::const_iterator it) = 0;

        //Adds the unfixed knot points of the spine curve to the list
        virtual void getUnfixedKnotPoints(std::list<double>& points) const = 0;

		/// Prints the curve
        virtual std::ostream& print(std::ostream& out) const = 0;
    }; 

    DECLARE_SMART_PTRS( ICurve )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const ICurve& iCurve)
		{
			return iCurve.print(out);
		}
	}
}
#endif //__LIBRARY_PRICERS_FLEXYCF_ICURVE_H_INCLUDED