/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CURVEALGORITHM_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CURVEALGORITHM_H_INCLUDED
#pragma once

#include "Gradient.h"
#include "KnotPoint.h"
#include "KnotPoints.h"

namespace FlexYCF
{
    /// CurveAlgorithm is a base class that exposes an interface that 
    /// all concrete curve algorithms (e.g. interpolation and 
    /// extrapolation methods) must implement.
    class CurveAlgorithm
    {
    public:
        virtual ~CurveAlgorithm() { }
        typedef KnotPointConstIterator const_iterator;
        
        /// Evaluates the curve at the point x
        virtual double evaluate(const double x) const = 0;

        /// Computes the gradient relative to the unknowns
        /// and adds multiplier times the gradient to the points between the iterators supplied
        virtual void accumulateGradient(const double x,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd) const = 0; 

        /// Computes the gradient relative to the unknowns
        /// and adds multiplier times the gradient to the points between the iterators supplied
        virtual void accumulateIntegralGradient(const double /* x */,
                                                double /* multiplier */,
                                                GradientIterator /* gradientBegin */,
                                                GradientIterator /* gradientEnd */) const
        {
            LT_THROW_ERROR("Not implemented by default");
        }

        /// Computes the integral of the curve from the smallest knot to x
        virtual double computeIntegral(const double /* x */) const
        {
            LT_THROW_ERROR("Not implemented by default");
        }


        /// Sets the m_begin and m_end iterators. 
        /// Usage note: should be called once all the knot-points have been added to the curve, 
        /// and before calling evaluate / computeGradient
        /// Note: calls onExtremalIteratorsSet once those iterators have been set
        void setExtremalIterators(const const_iterator beginIterator, 
                                  const const_iterator endIterator);

        
    protected:
        inline const_iterator begin() const
        {   return m_begin; }

        inline const_iterator end() const
        {   return m_end;   }
        
        size_t size() const;

        // A hook function that complex derived curve algorithms can 
        //  override to do pre-calculations 
        virtual void onExtremalIteratorsSet()
        {
        }

    private:
        /// Any curve algorithm only has access to the first and
        /// one past last iterators pointing to the knot-points
        /// of the curve
        const_iterator m_begin;
        const_iterator m_end;
    };  //  CurveAlgorithm

    DECLARE_SMART_PTRS( CurveAlgorithm )
}   //  FlexYCF

#endif // __LIBRARY_PRICERS_FLEXYCF_CURVEALGORITHM_H_INCLUDED