/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FLATRIGHTINTERPOLATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FLATRIGHTINTERPOLATION_H_INCLUDED
#pragma once

#include "InterpolationMethod.h"


namespace FlexYCF
{
    /// A class to perform flat right interpolation:
    /// if the knots are given by {(x[i], y[i]); 0 <= i <= n}
    /// then interpolation is f(x) = y[i-1] if x is such that
    /// x[i-1] <= x < x[i]
    class FlatRightInterpolation: public InterpolationMethod
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

     DECLARE_SMART_PTRS( FlatRightInterpolation )

}   //  FlatRightInterpolation
#endif //__LIBRARY_PRICERS_FLEXYCF_FLATRIGHTINTERPOLATION_H_INCLUDED