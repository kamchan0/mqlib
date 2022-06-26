/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SPOTRATECURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SPOTRATECURVE_H_INCLUDED
#pragma once
#include "CurveFormulation.h"
#include "Gradient.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    /// SpotRateCurve represents the function that maps a 
    /// future time (in years) t to the spot rate r(t) such 
    /// such that the corresponding discount factor P(t)
    /// is equal to:
    ///          P(t) = exp(- t * r(t))
    ///
    /// Note: the spot rate curve is also called the zero-
    /// coupon yield curve.
    class SpotRateCurve : public CurveFormulation
    {
    public:
        explicit SpotRateCurve(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                               const std::string& curveDescription,
                               const LeastSquaresResidualsPtr& leastSquaresResiduals);

        static std::string getName();
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

    protected:
        SpotRateCurve(SpotRateCurve const& original, CloneLookup& lookup);

    private:
        SpotRateCurve(SpotRateCurve const&); // deliberately disabled as won't clone properly
    };
}
#endif //__LIBRARY_PRICERS_FLEXYCF_SPOTRATECURVE_H_INCLUDED