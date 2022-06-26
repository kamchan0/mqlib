/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DISCOUNTCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DISCOUNTCURVE_H_INCLUDED
#pragma once

#include "CurveFormulation.h"
#include "Gradient.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    /// DiscountCurve represents the function that associates
    /// to each future time (in years) t its discount factor P(t).
    /// 
    /// Important note: This does not necessary represent the 
    /// value of one unit of a Currency at a future time, as a 
    /// model can multiply the discount factors retrieved from
    /// this curve for, say, account for a spread.
    /// Rather, this class should be seen a mean to formulate
    /// the general problem using discount factors.
    class DiscountCurve : public CurveFormulation
    {
    public:
        explicit DiscountCurve(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                               const std::string& curveDescription,
                               const LeastSquaresResidualsPtr& leastSquaresResiduals);

        static std::string getName();

        /// Creates a DiscountCurve
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
        // virtual void addInitialSpotRate(const double flowTime, 
        //                                 const double spotRate) const;

        virtual void initializeKnotPoints() const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        DiscountCurve(DiscountCurve const& original, CloneLookup& lookup);

    private:
        virtual void onFinalize() const;    
        DiscountCurve(DiscountCurve const&); // deliberately disabled as won't clone properly

        double spotRateInitializer(const double x) const
        {
            return exp(- x * getInitSpotRate());
        }
    };
}

#endif //__LIBRARY_PRICERS_FLEXYCF_DISCOUNTCURVE_H_INCLUDED