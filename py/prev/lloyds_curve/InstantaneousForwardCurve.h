/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INSTANTANEOUSFORWARDCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INSTANTANEOUSFORWARDCURVE_H_INCLUDED
#pragma once
#include "CurveFormulation.h"
#include "Gradient.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    /// InstantenousForwardCurve represents the curve,
    /// not necessarily continuous, that maps a future 
    /// time (in years) t to the instantaneous
    /// forward rate f(t), such that the corresponding  
    /// discount factor P(t) verifies:          
    ///     P(t) = exp(- integral from 0 to t of f)    
    class InstantaneousForwardCurve : public CurveFormulation
    {
    public:
        explicit InstantaneousForwardCurve(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                           const std::string& curveDescription,
                                           const LeastSquaresResidualsPtr& leastSquaresResiduals);

        static std::string getName();

        /// Creates an InstantaneousForwardCurve
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
        InstantaneousForwardCurve(InstantaneousForwardCurve const& original, CloneLookup& lookup);

    private:
        InstantaneousForwardCurve(InstantaneousForwardCurve const&); // deliberately disabled as won't clone properly
    };
}
#endif //__LIBRARY_PRICERS_FLEXYCF_INSTANTANEOUSFORWARDCURVE_H_INCLUDED