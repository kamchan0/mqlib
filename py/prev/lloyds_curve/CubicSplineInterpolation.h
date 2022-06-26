/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CUBICSPLINEINTERPOLATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CUBICSPLINEINTERPOLATION_H_INCLUDED
#pragma once

#include "InterpolationMethod.h"

namespace FlexYCF
{
    /// Not implemented:
    /// Use a TensionSpline with tension parameters all set to 0.0
    /// to emulate cubic spline.
    class CubicSplineInterpolation: public InterpolationMethod
    {
    public:
        virtual double evaluate(const double x) const;
        virtual void accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const; 
        InterpolationMethodPtr clone() const;    
    };

     DECLARE_SMART_PTRS(CubicSplineInterpolation)
}
#endif //__LIBRARY_PRICERS_FLEXYCF_CUBICSPLINEINTERPOLATION_H_INCLUDED