/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDFORWARDRATE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDFORWARDRATE_H_INCLUDED
#pragma once

#include "DiscountedForwardRateArguments.h"
#include "FloatingLegCashFlow.h"
#include "Gradient.h"

namespace IDeA
{
    class AssetDomain;
}

namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( DiscountedForwardRate )

    /// DiscountForwardRate represents
    /// the discounted forward rate times year fraction 
    /// between the set date t and the pay date T:
    ///   cvg(t, T)  *  F(t, T; tenor)  *  DF(0, T)
    class DiscountedForwardRate : public FloatingLegCashFlow
    {
    public:
        typedef DiscountedForwardRateArguments Arguments;

        explicit DiscountedForwardRate(const DiscountedForwardRateArguments& arguments):
            m_arguments(arguments),
            m_forwardRate(arguments.getForwardRate()),
            m_discountFactor(arguments.getDiscountFactor()),
            m_coverage(arguments.getCoverage())
        {
        }

        static DiscountedForwardRatePtr create(const DiscountedForwardRateArguments& arguments)
        {
            return DiscountedForwardRatePtr(new DiscountedForwardRate(arguments));
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
		virtual double getRate(const BaseModel& model);
		
		virtual void update();

		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
                                  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);

         // provided for compatibility with DiscountedForwardRate
        const Arguments& getArguments() const
        {
            return m_arguments;
        }

        bool operator==(const DiscountedForwardRate& other) const
        {
            return (m_arguments == other.m_arguments);
        }
        
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        DiscountedForwardRate(DiscountedForwardRate const& original, CloneLookup& lookup);

    private:
        DiscountedForwardRate(DiscountedForwardRate const&); // deliberately disabled as won't clone properly

        const Arguments m_arguments;
        const ForwardRatePtr  m_forwardRate;
        const DiscountFactorPtr m_discountFactor;
        const double m_coverage;
    };  // DiscountedForwardRate

	size_t hash_value(const DiscountedForwardRatePtr discountedForwardRate);
}   //  FlexYCF

namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( DiscountedForwardRateNotionalExchange )

    class DiscountedForwardRateNotionalExchange : public MultiCcyFloatingLegCashFlow
    {
    public:
        typedef DiscountedForwardRateNotionalExchangeArguments Arguments;

        explicit DiscountedForwardRateNotionalExchange(const DiscountedForwardRateNotionalExchangeArguments& arguments):
            m_arguments(arguments),
            m_forwardRate(arguments.getForwardRate()),
            m_discountFactor(arguments.getDiscountFactor()),
            m_startDateDiscountFactor(arguments.getStartDateDiscountFactor()),
            m_foreignStartDateDiscountFactor(arguments.getForeignStartDateDiscountFactor()),
            m_domesticFixingDateDiscountFactor(arguments.getDomesticFixingDateDiscountFactor()),
            m_foreignFixingDateDiscountFactor(arguments.getForeignFixingDateDiscountFactor()),
		    m_domesticSpotFxDateDiscountFactor(arguments.getDomesticSpotFxDateDiscountFactor()),
            m_foreignSpotFxDateDiscountFactor(arguments.getForeignSpotFxDateDiscountFactor()),
            m_coverage(arguments.getCoverage())
        {
        }

        static DiscountedForwardRateNotionalExchangePtr create(const DiscountedForwardRateNotionalExchangeArguments& arguments)
        {
            return DiscountedForwardRateNotionalExchangePtr(new DiscountedForwardRateNotionalExchange(arguments));
        }

        virtual double getValue(BaseModel const& domModel, BaseModel const& forModel, double fx);
         
        virtual double getNotional(BaseModel const& domModel, BaseModel const& forModel, double fx);
        
        
        virtual void accumulateGradient(BaseModel const& domModel, BaseModel const& forModel, double fx,
										double multiplier,
										GradientIterator gradientBegin,
										GradientIterator gradientEnd);

		virtual void accumulateGradient(BaseModel const& domModel, BaseModel const& forModel, double fx,
										double multiplier,
										GradientIterator gradientBegin,
										GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);
		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
									    double multiplier,
                                        double fx, 
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread);
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
									    double multiplier,
                                        double fx, 
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomainDom,
                                  const BaseModel& domModel,
								  IDeA::AssetDomainConstPtr assetDomainFor,
                                  const BaseModel& forModel,
								  double spot,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomainDom,
                                  const BaseModel& domModel,
								  IDeA::AssetDomainConstPtr assetDomainFor,
                                  const BaseModel& forModel,
								  double spot,
								  const double multiplier,
                                  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);

		virtual double getRate(BaseModel const& domModel, BaseModel const& forModel, double fx);
		
		virtual void update();

        const Arguments& getArguments() const
        {
            return m_arguments;
        }

        bool operator==(const DiscountedForwardRateNotionalExchange& other) const
        {
            return (m_arguments == other.m_arguments);
        }
        
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        DiscountedForwardRateNotionalExchange(DiscountedForwardRateNotionalExchange const& original, CloneLookup& lookup);

    private:
        DiscountedForwardRateNotionalExchange(DiscountedForwardRateNotionalExchange const&); // deliberately disabled as won't clone properly

        const Arguments m_arguments;
        const ForwardRatePtr  m_forwardRate;
        const DiscountFactorPtr m_discountFactor;
        const DiscountFactorPtr m_startDateDiscountFactor;
        const DiscountFactorPtr m_foreignStartDateDiscountFactor;
        const DiscountFactorPtr m_domesticFixingDateDiscountFactor;
        const DiscountFactorPtr m_foreignFixingDateDiscountFactor;
		const DiscountFactorPtr m_domesticSpotFxDateDiscountFactor;
        const DiscountFactorPtr m_foreignSpotFxDateDiscountFactor;
        const double m_coverage;
    };  

	size_t hash_value(const DiscountedForwardRateNotionalExchangePtr discountedForwardRate);
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDFORWARDRATE_H_INCLUDED