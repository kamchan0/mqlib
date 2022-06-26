/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FLOATINGLEG_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FLOATINGLEG_H_INCLUDED
#pragma once
#include "FloatingLegArguments.h"
#include "Gradient.h"
#include "BaseLeg.h"
#include "DiscountedOISCashflow.h"

namespace IDeA
{
    class AssetDomain;
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( FloatingLeg )
    FWD_DECLARE_SMART_PTRS( FloatingLegCashFlow )

    /// Represents a floating leg whose cash-flows can be either
    /// of type DiscountedForwardRate or FixedCashFlow.
    class FloatingLeg : public InstrumentComponent, 
                        public ICloneLookup,
                        public BaseLeg<FloatingLegArguments>
    {
    private:
        typedef std::vector<FloatingLegCashFlowPtr> CashFlowContainer;
        typedef CashFlowContainer::const_iterator const_iterator;

    public:
        typedef FloatingLegArguments Arguments;

        explicit FloatingLeg(const FloatingLegArguments& arguments);

        static FloatingLegPtr create(const FloatingLegArguments& arguments)
        {
            return FloatingLegPtr(new FloatingLeg(arguments));
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
		virtual void update();

		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);

        virtual std::ostream& print(std::ostream& out) const;        
    
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

        virtual void cleanupCashFlows();

    protected:
        FloatingLeg(FloatingLeg const& original, CloneLookup& lookup);

    private:
         // initialize the cash-flows
        //	void    initialize(const FloatingLegArguments& arguments);
        FloatingLeg(FloatingLeg const&);
		
		//	BaseLeg interface implementation
		virtual std::string getLegTypeName() const;
		virtual void initializeCashFlows();
		virtual void doInitializeCashFlowPricing();
		virtual void fillSingleCashFlowPV(const size_t index,
										  const LT::date startDate,
										  const LT::date endDate,
										  const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable);

		DiscountedForwardRatePtr createDiscountedForwardRate(const LT::date valueDate,
															 const LT::date accrualStartDate,
															 const LT::date accrualEndDate,
															 const LTQC::Tenor& payDelay,
															 const LT::Str& payCalendar,
                                                             const ModuleDate::DayCounterConstPtr& rateBasis,
															 const ModuleDate::DayCounterConstPtr& accrualBasis,
															 const IDeA::DepositRateMktConvention& rateDetails,
															 const bool isLastPeriod) const;

		DiscountedOISCashflowPtr createDiscountedOISCashflow(const LT::date& valueDate,
															 const LT::date& accrualStartDate,
															 const LT::date& accrualEndDateconst,
															 const LTQC::Tenor& payDelay,
															 const LT::Str& payCalendar,
															 ModuleDate::DayCounterConstPtr basisON,
															 ModuleDate::DayCounterConstPtr accrualBasis,
                                                             const LTQC::Currency& ccy, 
                                                             const LT::Str& index) const;

        DiscountedArithmeticOISCashflowPtr createDiscountedArithmeticOISCashflow(const LT::date& valueDate,
															 const LT::date& accrualStartDate,
															 const LT::date& accrualEndDateconst,
															 const LT::Str& accrualCalendar,
															 const LTQC::Tenor& payDelay,
															 const LT::Str& payCalendar,
															 ModuleDate::DayCounterConstPtr basisON,
															 ModuleDate::DayCounterConstPtr accrualBasis,
															 size_t cutoff,
                                                             const LTQC::Currency& ccy, 
                                                             const LT::Str& index) const;

        FundingDiscountedCashflowPtr createFundingDiscountedCashflow(const LT::date& valueDate,
															 const LT::date& accrualStartDate,
															 const LT::date& accrualEndDate,
															 ModuleDate::DayCounterConstPtr accrualBasis,
                                                             const LTQC::Currency& ccy, 
                                                             const LT::Str& index) const;
        //  DiscountedForwardRateContainer  
        CashFlowContainer   m_cashflows;    
    };     // FloatingLeg

    DECLARE_SMART_PTRS( FloatingLeg )

}   //  FlexYCF



namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( MultiCcyFloatingLeg )
    FWD_DECLARE_SMART_PTRS( MultiCcyFloatingLegCashFlow )

   
    class MultiCcyFloatingLeg : public MultiCcyInstrumentComponent, 
                                public ICloneLookup,
                                public BaseLeg<FloatingLegArguments>
    {
    private:
        typedef std::vector<MultiCcyFloatingLegCashFlowPtr> CashFlowContainer;
        typedef CashFlowContainer::const_iterator const_iterator;

    public:
        typedef FloatingLegArguments Arguments;

        explicit MultiCcyFloatingLeg(const FloatingLegArguments& arguments);

        static MultiCcyFloatingLegPtr create(const FloatingLegArguments& arguments)
        {
            return MultiCcyFloatingLegPtr(new MultiCcyFloatingLeg(arguments));
        }

        
        virtual double getValue(const BaseModel& domModel, const BaseModel& forModel, double fx);
        
        virtual void accumulateGradient(const BaseModel& domModel, const BaseModel& forModel, double fx, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd);
		
        virtual void accumulateGradient(const BaseModel& domModel, const BaseModel& forModel, double fx, 
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

        void fillSingleCashFlowPV(const size_t index, const LT::date startDate, const LT::date endDate, const BaseModel& domModel, const BaseModel& forModel, double fx, LTQuant::GenericData& cashFlowPVsTable);

        LTQuant::GenericDataPtr computeCashFlowPVs(const BaseModel& domModel, const BaseModel& forModel, double fx)
	    {
		    initializeCashFlowPricing();

		    const LTQuant::GenericDataPtr result(new LTQuant::GenericData(getLegTypeName(), 0));
		    size_t index(0);

            for(ModuleDate::Schedule::ScheduleEvents::const_iterator iter(m_scheduleDates.begin()); iter != m_scheduleDates.end(); ++iter, ++index)
		    {
			    result->set<LT::date>("Accrual Start Date", index, iter->begin());
			    result->set<LT::date>("Accrual End Date", index, iter->end());
                result->set<double>("Discount Factor Start Date", index, domModel.getDiscountFactor(ModuleDate::getYearsBetween(domModel.getValueDate(), iter->begin())));
                result->set<double>("Discount Factor End Date", index, domModel.getDiscountFactor(ModuleDate::getYearsBetween(domModel.getValueDate(), iter->end())));
			    fillSingleCashFlowPV(index, iter->begin(), iter->end(), domModel, forModel, fx, *result);
		    }

		    return result;
	    }

        virtual void update();

		
        virtual std::ostream& print(std::ostream& out) const;        
    
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

        virtual void cleanupCashFlows();

    protected:
        MultiCcyFloatingLeg(MultiCcyFloatingLeg const& original, CloneLookup& lookup);

    private:
        
        MultiCcyFloatingLeg(MultiCcyFloatingLeg const&);
		
		//	BaseLeg interface implementation
		virtual std::string getLegTypeName() const;
		virtual void initializeCashFlows();
		virtual void doInitializeCashFlowPricing();
		virtual void fillSingleCashFlowPV(const size_t index,
										  const LT::date startDate,
										  const LT::date endDate,
										  const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable);

		DiscountedForwardRateNotionalExchangePtr createDiscountedForwardRateNotionalExchange(const LT::date valueDate,
															 const LT::date accrualStartDate,
															 const LT::date accrualEndDate,
                                                             const ModuleDate::DayCounterConstPtr& rateBasis,
															 const ModuleDate::DayCounterConstPtr& accrualBasis,
															 const IDeA::DepositRateMktConvention& rateDetails,
															 const bool isLastPeriod) const;

		
        CashFlowContainer   m_cashflows;    
    }; 

    DECLARE_SMART_PTRS( MultiCcyFloatingLeg )

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_FLOATINGLEG_H_INCLUDED