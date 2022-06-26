/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHOD_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHOD_H_INCLUDED
#pragma once
#include "CurveAlgorithm.h"
#include "Gradient.h"


namespace FlexYCF
{
    
    /// ExtrapolationMethod is a common interface for 
    /// LeftExtrapolationMethod and RightExtrapolationMethod
    class ExtrapolationMethod : public CurveAlgorithm
    {
    public:
        virtual ~ExtrapolationMethod() = 0 { }   
        
        void setInterpolationGradientFunction(const GradientFunction& interpolationGradientFunction)
        {
            m_interpolationGradientFunction = interpolationGradientFunction;
        }
        
    protected:
        /// Computes the gradient for an x that lies
        /// between first and last knots
        virtual void accumulateInterpolationGradient(const double x, 
                                                     double multiplier,
                                                     GradientIterator gradientBegin,
                                                     GradientIterator gradientEnd) const;

    private:
        /// a Boost function that calculates the 
        /// gradient at an x within the knots:
        GradientFunction m_interpolationGradientFunction;
    };

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHOD_H_INCLUDED