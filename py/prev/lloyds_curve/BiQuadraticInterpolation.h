/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BIQUADRATICINTERPOLATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BIQUADRATICINTERPOLATION_H_INCLUDED
#pragma once

#include "InterpolationMethod.h"


namespace FlexYCF
{
    /// BiQuadraticInterpolation is defined as follows.
    /// Given a collection of points {(x[i], y[i]); 0 <= i <= N},
    /// define, for 1 <= i <= N-1,  the quadratic polynomial Q[i](x)
    /// going through the three points (x[k], y[k]), i-1 <= k <= i+1.
    /// In addtion define Q[0] (respectively Q[N]) as the linear function 
    /// going through the points (x[0], y[0]) and (x[1], y[1]) (resp.
    /// though (x[N-1], y[N-1]) and (x[N], y[N]).
    /// Also set l[i](x) := (x - x[i-1]) / (x[i] - x[i-1]).
    /// Then for x in [x[i-1], x[i]), bi-quadratic interpolation at x, 
    /// is computed as:
    ///
    ///     l[i](x) * Q[i](x) + (1 - l[i-1](x)) * Q[i-1](x)
    ///
    /// Notes:
    /// - It can be shown that for 1 <= i <= N-1, the coefficients of
    /// the quadratic polynomial Q[i](x) = a[i]*x*x + b[i]*x + c[i], are
    /// with: 
    ///     a[i] = (sl[i+1] - sl[i]) / (x[i+1] - x[i-1])
    ///     b[i] = sl[i] - a[i] * (x[i-1] + x[i])
    ///     c[i] = y[i-1] + x[i-1] * (a[i] * x[i] - sl[i])
    /// where we set sl[i] := (y[i] - y[i-1]) / (x[i] - x[i-1]); this is
    /// the slope between (x[i-1], y[i-1]) and (x[i], y[i])
    /// - for i = 0 and i = N, a[i] = 0 and it is possible to choose
    ///     between a constant and a straight line degenerate quadratic
    ///     such that Q[0](x[0]) = y[0] and Q[N](x) = y[N]. By default,
    ///     Q[0] is constant and Q[N] is straight line.
    /// The following will be obsolete once extrapolation methods are integrated:
    /// - On the left of the first knot-point, just flat
    /// - On the right of the last knot-point, just carries on the slope
    class BiQuadraticInterpolation : public InterpolationMethod
    {
    public:
        typedef enum
        {
            CONSTANT,
            LINEAR
        } eExtremeInterpolationType;

        BiQuadraticInterpolation(const eExtremeInterpolationType firstQuadratic = CONSTANT,
                                 const eExtremeInterpolationType lastQuadratic = LINEAR);
  
        static std::string getName()
        {
            return "BiQuadratic";
        }

        virtual void onExtremalIteratorsSet();
        virtual double evaluate(const double x) const;

        /// Computes the gradient relative to the unknowns
        /// and adds multiplier times the gradient to the points between the iterators supplied
        virtual void accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const; 

        InterpolationMethodPtr clone() const;    

    private:
        double computeAlpha(const size_t index) const;
        double computeBeta(const size_t index, const double alpha) const;
        double computeGamma(const size_t index, const double alpha) const;

        eExtremeInterpolationType m_firstQuadratic;
        eExtremeInterpolationType m_lastQuadratic;

        std::vector<std::vector<double> > m_alphaGrad;
        std::vector<std::vector<double> > m_betaGrad;
        std::vector<std::vector<double> > m_gammaGrad;
      
    };  //  BiQuadraticInterpolation
    
    DECLARE_SMART_PTRS(BiQuadraticInterpolation);

}   // FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_BIQUADRATICINTERPOLATION_H_INCLUDED