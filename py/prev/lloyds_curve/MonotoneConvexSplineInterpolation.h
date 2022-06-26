/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_MONOTONECONVEXSPLINEINTERPOLATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MONOTONECONVEXSPLINEINTERPOLATION_H_INCLUDED
#pragma once

#include "InterpolationMethod.h"

namespace FlexYCF
{
    /// This interpolation is from Hagan & West
    /// The value at the knot-point must represent the integral of the
	/// instantaneous forward rate from 0 to its x (time). This corresponds
	///	to a "LogFvf" formulation.
    /// Extrapolation is flat on the left and the right of knot-points.
    /// If there are only 3 knot-points or less, piecewise linear (straight line)
    /// interpolation is performed.
    class MonotoneConvexSplineInterpolation: public InterpolationMethod
    {
    public:
        static std::string getName();

        // If enforcePositivity is true, the forward will be > 0
        // This never seems to be a problem in practice so this is not used
		explicit MonotoneConvexSplineInterpolation(const bool enforcePositivity = false, 
												   const double positivityCoef = 2.0);
        virtual double evaluate(const double x) const;

        /// Computes the gradient relative to the unknowns
        /// and adds multiplier times the gradient to the points between the iterators supplied
        virtual void accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const; 

        InterpolationMethodPtr clone() const;    

        virtual void onExtremalIteratorsSet();
        virtual void update();

        // getters / setters
        inline
        const bool getEnforcePositivity() const;
        inline
        void setEnforcePositivity(const bool enforcePositivity);

        inline
        const double getPositivityCoef() const;
        void setPositivityCoef(const double positivityCoef);

        virtual std::ostream& print(std::ostream& out) const;
    
    private:
        // might be worth to introduce a structure to group all these together as they all have the same size (+/-1)
        // See FlexYCF Technical note or Hagan & West paper to for more details on those variables
        std::vector<double> m_df;    //  discrete forwards : df[i] = (kp[i].y - kp[i-1].y) / (kp[i].x - kp[i-1].x)
        std::vector<double> m_f;     //  weighted average of forwards
        std::vector<double> m_K;     //  coefficients .. 
        std::vector<double> m_L;     //  .. for the ..    
        std::vector<double> m_M;     //  .. function f

        // m_gL and m_gR (for g-Left and g-Right resp.) correspond to the gradients relative to the knot-points y-coordinates of
        // the values g[i-1] and g[i] in Hagan & West paper for each interval of the curve. g[i-1] and g[i] are the values of the
        // function g(t) = f(t) - df[i] on the end-points of the interval [t[i-1], t[i][. However the notation is ambiguous in the
        // paper as g[i-1] and g[i] are only relative to the interval [t[i-1], t[i][
        // Each element of g-Left and g-Right corresponds to a gradient. As only three partial derivatives at most are non-zero,
        // only those that may not be zero are represented, the index of the knot-points they refer to being clear from the context.
        // The gradients on the first interval for g-Left and the last interval for g-Right are computed differently than the others.
        std::vector<std::vector<double> > m_gL;  
        std::vector<std::vector<double> > m_gR;

        bool m_enforcePositivity;
        // the coefficent such that we require  0 < f_iMinus1 < posCoef * discFwd_i and 0 < f_i < posCoef * discFwd_i
        double m_positivityCoef;   // 3.0 is sufficient, but H&W suggest 2.0 "to remain a reasonable distance away from 0"
        
            
        void computeKLM(const size_t idx);
        
        // This is to enforce positivity of the interpolant (see Hagan & West pp 25, 26)
        void enforceInterpolantPositivity();

        
        // returns whether x belongs to the interval [0, positivityCoef * m_df[i]
        inline
        const bool liesWithinRange(const double x, const size_t index) const;

        void shift_f(const size_t index);

        // return the local minimum of f on interval [t[i-1], t[i][ when g([i-1] > 0 and g[i] > 0
        inline
        const double f_local_minimum(const size_t index) const;
        
        // set fwd to min(max(0.0, fwd), max), so that it lies in [0.0, Max]
        // and return true if it was outside this interval
        // *********** NOT SURE WE USE IT ********************
        // const bool bound(double& fwd, const double Max);
    };

     DECLARE_SMART_PTRS(MonotoneConvexSplineInterpolation)
}
#endif //__LIBRARY_PRICERS_FLEXYCF_MONOTONECONVEXSPLINEINTERPOLATION_H_INCLUDED