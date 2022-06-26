#ifndef __LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDOISCASHFLOW_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDOISCASHFLOW_H_INCLUDED
#pragma once

#include "DiscountedOISCashflowArguments.h"
#include "DiscountFactor.h"
#include "FloatingLegCashFlow.h"
#include "Gradient.h"
#include "ModuleDate/InternalInterface/Utils.h"
#include "TenorDiscountFactor.h"

namespace IDeA
{
    class AssetDomain;
}

namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( DiscountedOISCashflow )

    //   cvg(start, end)  *  F(start,end)  *  df(pay) = cvg/cvgON * (df(start) - df(end))*df(pay)/df(end)
	// F = (df(start)/df(end)-1)/cvgON(start,end)
    class DiscountedOISCashflow : public FloatingLegCashFlow
    {
    public:
        typedef DiscountedOISCashflowArguments Arguments;

        explicit DiscountedOISCashflow(const DiscountedOISCashflowArguments& arguments):
            m_arguments(arguments),
			m_payDiscountFactor(DiscountFactor::create(DiscountFactorArguments( arguments.getValueDate(), arguments.getPayDate(), arguments.getCurrency(), arguments.getIndex() ) )),
            m_startDateTenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(arguments.getValueDate(), arguments.getStartDate() , "1b", arguments.getCurrency(), arguments.getIndex()))),
            m_endDateTenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(arguments.getValueDate(), arguments.getEndDate() , "1b", arguments.getCurrency(), arguments.getIndex())))
        {
        }

        static DiscountedOISCashflowPtr create(const DiscountedOISCashflowArguments& arguments)
        {
            return DiscountedOISCashflowPtr(new DiscountedOISCashflow(arguments));
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

        bool operator==(const DiscountedOISCashflow& other) const
        {
            return (m_arguments == other.m_arguments);
        }
        
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        DiscountedOISCashflow(DiscountedOISCashflow const& original, CloneLookup& lookup);

    private:
        DiscountedOISCashflow(DiscountedOISCashflow const&); // deliberately disabled as won't clone properly

        const Arguments m_arguments;
		const DiscountFactorPtr m_payDiscountFactor;

        TenorDiscountFactorPtr	m_startDateTenorDiscountFactor;
        TenorDiscountFactorPtr	m_endDateTenorDiscountFactor;

    };  
}   //  FlexYCF

namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( FundingDiscountedCashflow )

    class FundingDiscountedCashflow : public FloatingLegCashFlow
    {
    public:
        typedef FundingDiscountedCashflowArguments Arguments;

        explicit FundingDiscountedCashflow(const FundingDiscountedCashflowArguments& arguments):
            m_arguments(arguments),
            m_startDateDiscountFactor(DiscountFactor::create(DiscountFactorArguments(arguments.getValueDate(), arguments.getStartDate(), arguments.getCurrency(), arguments.getIndex()))),
            m_endDateDiscountFactor(DiscountFactor::create(DiscountFactorArguments(arguments.getValueDate(), arguments.getEndDate(), arguments.getCurrency(), arguments.getIndex()))),
            m_payDateDiscountFactor(DiscountFactor::create(DiscountFactorArguments(arguments.getValueDate(), arguments.getEndDate(), arguments.getCurrency())))
        {
        }

        static FundingDiscountedCashflowPtr create(const FundingDiscountedCashflowArguments& arguments)
        {
            return FundingDiscountedCashflowPtr(new FundingDiscountedCashflow(arguments));
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

        const Arguments& getArguments() const
        {
            return m_arguments;
        }

        bool operator==(const FundingDiscountedCashflow& other) const
        {
            return (m_arguments == other.m_arguments);
        }
        
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        FundingDiscountedCashflow(FundingDiscountedCashflow const& original, CloneLookup& lookup);

    private:
        FundingDiscountedCashflow(FundingDiscountedCashflow const&); // deliberately disabled as won't clone properly
        
        void initialize(const BaseModel& model) const;
        
        const Arguments m_arguments;
	

        DiscountFactorPtr	m_startDateDiscountFactor;
        DiscountFactorPtr	m_endDateDiscountFactor;
        DiscountFactorPtr	m_payDateDiscountFactor;
        mutable BaseModelConstPtr               m_fundingFwdModel;
		mutable IDeA::AssetDomainConstPtr 	     	m_fundingFwdModelAD;
    };  
}   //  FlexYCF

namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( DiscountedArithmeticOISCashflow )

    //   cvg(start, end)  *  F(start,end)  *  df(pay) = cvg/cvgON * (df(start) - df(end))*df(pay)/df(end)
	// F = (df(start)/df(end)-1)/cvgON(start,end)
    class DiscountedArithmeticOISCashflow : public FloatingLegCashFlow
    {
    public:
        typedef DiscountedArithmeticOISCashflowArguments Arguments;

        explicit DiscountedArithmeticOISCashflow(const Arguments& arguments):
            m_arguments(arguments),
			m_payDiscountFactor(DiscountFactor::create(DiscountFactorArguments( arguments.getValueDate(), arguments.getPayDate(), arguments.getCurrency(), arguments.getIndex() ) )),
            m_startDateTenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(arguments.getValueDate(), arguments.getStartDate() , "1b", arguments.getCurrency(), arguments.getIndex()))),
            m_endDateTenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(arguments.getValueDate(), arguments.getEndDate() , "1b", arguments.getCurrency(), arguments.getIndex()))),
			m_cutoffAdj(arguments.cutOffAdj())
        {
			const std::vector<LT::date>& endDates = arguments.getEndDates();
			for(size_t i=0; i < endDates.size(); ++i)
			{
				 m_endDatesTenorDiscountFactor.push_back(TenorDiscountFactor::create(TenorDiscountFactorArguments(arguments.getValueDate(), endDates[i] , "1b", arguments.getCurrency(), arguments.getIndex())));
			}
        }

        static DiscountedArithmeticOISCashflowPtr create(const Arguments& arguments)
        {
            return DiscountedArithmeticOISCashflowPtr(new DiscountedArithmeticOISCashflow(arguments));
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

        bool operator==(const DiscountedArithmeticOISCashflow& other) const
        {
            return (m_arguments == other.m_arguments);
        }
        
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        DiscountedArithmeticOISCashflow(DiscountedArithmeticOISCashflow const& original, CloneLookup& lookup);

    private:
        DiscountedArithmeticOISCashflow(DiscountedArithmeticOISCashflow const&); // deliberately disabled as won't clone properly

        const Arguments m_arguments;
		const DiscountFactorPtr m_payDiscountFactor;

        TenorDiscountFactorPtr	m_startDateTenorDiscountFactor;
        TenorDiscountFactorPtr	m_endDateTenorDiscountFactor;
		 
		std::vector<TenorDiscountFactorPtr>	m_endDatesTenorDiscountFactor;
		double m_cutoffAdj;
    };  
}   //  FlexYCF



#endif 