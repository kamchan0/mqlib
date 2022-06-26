/*****************************************************************************

    FixedCashFlow

	Represents a fixed cash-flow in a floating leg.
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FIXEDCASHFLOW_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FIXEDCASHFLOW_H_INCLUDED
#pragma once

//	FlexYCF
#include "FloatingLegCashFlow.h"
#include "FixedCashFlowArguments.h"
#include "DiscountFactor.h"
#include "Gradient.h"

namespace IDeA
{
    class AssetDomain;
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( FixedCashFlow )
    FWD_DECLARE_SMART_PTRS( BaseModel )


    /// Represents a fixed cash-flow in an 
    /// interest rate floating floating leg cash flow:
    /// Letting R be the simple fixed rate set at date t and paid
    /// at date T, and cvg(t, T) the corresponding year fraction,
    /// the value at date 0 of such a fixed cash-flow is:
    ///             cvg(t,T) * R * P(0, T)
    class FixedCashFlow : public FloatingLegCashFlow
    {
    public:
        typedef FixedCashFlowArguments Arguments;

        explicit FixedCashFlow(const FixedCashFlowArguments& arguments);

        static FixedCashFlowPtr create(const FixedCashFlowArguments& arguments)
        {
            return FixedCashFlowPtr(new FixedCashFlow(arguments));
        }

        virtual double getValue(BaseModel const& baseModel);
        virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd);
		virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);
		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);
		virtual double getRate(const BaseModel&);
        virtual void update();
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        FixedCashFlow(FixedCashFlow const& original, CloneLookup& lookup);

    private:
        FixedCashFlow(FixedCashFlow const&); // deliberately disabled as won't clone properly

        const Arguments m_arguments;                // here just for debug purpose, not necessary
        const DiscountFactorPtr m_discountFactor;   // in common with DiscountedForwardRate, could be factored to FloatingLegCashFlow
        const double m_coverageTimesRate;
    };

    DECLARE_SMART_PTRS( FixedCashFlow )
}

#endif //__LIBRARY_PRICERS_FLEXYCF_FIXEDCASHFLOW_H_INCLUDED