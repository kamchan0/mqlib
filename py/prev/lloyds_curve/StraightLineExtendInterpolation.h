/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_STRAIGHTLINEEXTENDINTERPOLATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_STRAIGHTLINEEXTENDINTERPOLATION_H_INCLUDED
#pragma once

#include "InterpolationMethod.h"


namespace FlexYCF
{
    /// StraightLineExtendInterpolation represents continuous
    /// piecewise linear interpolation between a set of
    /// points {(x[i], y[i]); 0 <= i <= N}.
    /// Namely, for any x in [x[i-1], x[i]), its interpolated 
    /// value is computed as:
    /// fi(x) = [(x[i] - x) * y[i-1] + (x - x[i-1]) * y[i])] / (x[i] - x[i-1]) 
    /// and the gradient (relative to the y's) is read on 
    /// this formula.
    /// Moreover, f(x) = x[0] for x < 0 and f(x) = fN(x) for x >= x[N]
    class StraightLineExtendInterpolation: public InterpolationMethod
    {
    public:
        static std::string getName();

        virtual double evaluate(const double x) const;

        /// Computes the gradient relative to the unknowns
        /// and adds multiplier times the gradient to the points between the iterators supplied
        virtual void accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const; 

        virtual double computeIntegral(const double x) const;

        /// Computes the gradient relative to the unknowns
        /// and adds multiplier times the gradient to the points between the iterators supplied
        virtual void accumulateIntegralGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;

        InterpolationMethodPtr clone() const;    
    };
    
    DECLARE_SMART_PTRS(StraightLineExtendInterpolation)

}   // FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_STRAIGHTLINEEXTENDINTERPOLATION_H_INCLUDED