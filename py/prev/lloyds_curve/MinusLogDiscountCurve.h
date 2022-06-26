/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_MINUSLOGDISCOUNTCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MINUSLOGDISCOUNTCURVE_H_INCLUDED
#pragma once

#include "CurveFormulation.h"
#include "Gradient.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    /// MinusLogDiscountCurve represents the function F that 
    /// associates minus the logarithm of the discount factor P(t),
    /// for each future time (in years) t: F(t) = - log(P(t)),
    /// such that: P(t) = exp( -F(t) ).
    ///
    /// Note: we must have F(0) = 0.0
    /// also, F(t) is the integral form 0 to t of the instantaneous
    /// forward curve.
    class MinusLogDiscountCurve : public CurveFormulation
    {
    public:
        explicit MinusLogDiscountCurve(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                       const std::string& curveDescription,
                                       const LeastSquaresResidualsPtr& leastSquaresResiduals);

        static std::string getName();

        /// Creates a MinusLogDiscountCurve
        static CurveFormulationPtr createInstance(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                  const std::string& curveDescription,
                                                  const LeastSquaresResidualsPtr& leastSquaresResiduals);

        virtual double getDiscountFactor(const double flowTime) const;
        virtual void accumulateDiscountFactorGradient(const double x,
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd) const;
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
														  const double discountFactor) const;
        virtual void initializeKnotPoints() const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

        virtual void setSpotRate(double spotTime, double spotRate)
        {
            m_spotTime = spotTime;
            m_spotRate = spotRate;
        }

    protected:
        MinusLogDiscountCurve(MinusLogDiscountCurve const& original, CloneLookup& lookup);

    private:
        virtual void onFinalize() const;
        MinusLogDiscountCurve(MinusLogDiscountCurve const&); // deliberately disabled as won't clone properly

        double m_spotTime;
        double m_spotRate;
    };
}

#endif //__LIBRARY_PRICERS_FLEXYCF_MINUSLOGDISCOUNTCURVE_H_INCLUDED