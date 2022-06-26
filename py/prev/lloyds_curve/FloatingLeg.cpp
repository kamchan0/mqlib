/*****************************************************************************
    
	FloatingLeg

	Implementation of the FloatingLeg class.

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "FloatingLeg.h"
#include "FloatingLegArguments.h"
#include "FloatingLegCashFlow.h"
#include "GlobalComponentCache.h"
#include "FlexYCFCloneLookup.h"

//	LTQuantLib
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"
#include "ModuleDate/InternalInterface/DayCounter.h"

// IDeA
#include "AssetDomain.h"
#include "Exception.h"

// LTQC
#include "DayCount.h"

#include "RollConv.h"
#include "dates/DateBuilderGenerator.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;

	FloatingLeg::FloatingLeg(const FloatingLegArguments& arguments):
		BaseLeg(arguments)
    { 
    //	    initialize(arguments);
    }

    double FloatingLeg::getValue(BaseModel const& baseModel)
    {
		initializeCashFlowPricing();
        
		double tmpCashFlowSum = 0.0;
        
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            tmpCashFlowSum += (*iter)->getValue(baseModel); 
        }

        return tmpCashFlowSum;
    }

    void FloatingLeg::accumulateGradient(BaseModel const& baseModel,
                                         double multiplier,
                                         GradientIterator gradientBegin,
                                         GradientIterator gradientEnd)
    {
		initializeCashFlowPricing();
        
        // compute the gradient of the first discounted forward rate in the floating leg   
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            // compute the gradient for this discounted forward rate:
            (*iter)->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
        }
    }

	void FloatingLeg::accumulateGradient(BaseModel const& baseModel, 
                                         double multiplier,
                                         GradientIterator gradientBegin, 
                                         GradientIterator gradientEnd,
										 const CurveTypeConstPtr& curveType)
	{
		initializeCashFlowPricing();
        
		// compute the gradient of the first discounted forward rate in the floating leg   
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            // compute the gradient for this discounted forward rate:
            (*iter)->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);
        }
	}
	
	void FloatingLeg::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                         double multiplier,
                                         GradientIterator gradientBegin,
                                         GradientIterator gradientEnd,
										 bool spread)
    {
		initializeCashFlowPricing();
        
        // compute the gradient of the first discounted forward rate in the floating leg   
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            // compute the gradient for this discounted forward rate:
            (*iter)->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier, gradientBegin, gradientEnd, spread);
        }
    }
	
	void FloatingLeg::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                         double multiplier,
                                         GradientIterator gradientBegin,
                                         GradientIterator gradientEnd,
										 bool spread)
    {
		initializeCashFlowPricing();
        
        // compute the gradient of the first discounted forward rate in the floating leg   
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            // compute the gradient for this discounted forward rate:
            (*iter)->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier, gradientBegin, gradientEnd, spread);
        }
    }

    void FloatingLeg::update()
    {
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            (*iter)->update();
        }
    }

	void FloatingLeg::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                   const BaseModel& model,
								   const double multiplier,
								   IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		initializeCashFlowPricing();

		for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
		{
			(*iter)->fillRepFlows(assetDomain, model, multiplier, fundingRepFlows);
		}
	}

	void FloatingLeg::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                   const BaseModel& model,
								   const double multiplier,
								   IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{		
		initializeCashFlowPricing();

		for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
		{
			(*iter)->fillRepFlows(assetDomain, model, multiplier, indexRepFlows);
		}
	}

	void FloatingLeg::initializeCashFlows()
	{
		if(m_scheduleDates.empty())
		{
			const string tenorDescription(arguments().getTenorDescription());
	        LTQC::Tenor tenor(tenorDescription == "ON" || tenorDescription == "1D" ? "1M" : tenorDescription);
			
			LTQC::DateBuilderGenerator floatingGen(LTQC::DateBuilder(arguments().getStartDate(), 
																	   arguments().getEndDate(), 
																	   tenor, 
																	   arguments().getCalendarString(), 
																	   arguments().getStubType(),
																	   Date(), 
																	   arguments().getRateDetails().m_rollConvention,    
                                                                       arguments().getRateDetails().m_rollRuleConvention, 
																	   LT::Str("1w"), 
																	   LTQC::RollConv(LTQC::RollConvMethod::None)));	
	       
			 // fill the payment dates
			floatingGen.fillEvents(m_scheduleDates, arguments().getStartDate(), arguments().getEndDate());
		}

	}
	
	void FloatingLeg::doInitializeCashFlowPricing()
	{
		if(m_cashflows.empty())
		{
            const string tenorDescription(arguments().getTenorDescription()); 
            const IDeA::DepositRateMktConvention& rateDetails = arguments().getRateDetails();
			//const bool isOISCashflow = rateDetails.m_rateTenor == LTQC::Tenor(0,1,0,0,0);

			//// consistency check
			//if ( !isOISCashflow && compareStrings(LT::Str(tenorDescription), rateDetails.m_rateTenor.asTenorString(), LT::compare_strings_flag_ignore_whitespace | LT::compare_strings_flag_caseless)) 
			//	LTQC_THROW(IDeA::MarketException,"Floating tenor " << tenorDescription << " must be the same as the index tenor " << rateDetails.m_rateTenor.asTenorString().data());

			const LT::date valueDate(arguments().getValueDate());
			const LT::date fixingDate(arguments().getFixingDate());
			const DayCounterConstPtr rateBasis(LTQC::DayCount::create(rateDetails.m_dcm));
			const DayCounterConstPtr accrualBasis(arguments().getBasis());

			// fill the cash-flows
			// the first cash-flow is fixed if Libor fixing is provided
			Schedule::ScheduleEvents::const_iterator iter(m_scheduleDates.begin());
			const size_t nbPeriods(m_scheduleDates.size());
			size_t periodCnt(1);

			// After the first cash-flow the cash-flows of the floating leg are discounted forward rates

			FloatingLegCashFlowPtr firstFloatFlowPtr;

			if (arguments().useLiborFixing()) 
			{
				if (InstrumentComponent::getUseCacheFlag()) 
				{
					firstFloatFlowPtr = getGlobalComponentCache()->get(FixedCashFlow::Arguments(valueDate, 
																			m_scheduleDates.begin()->begin(), 
																			m_scheduleDates.begin()->end(),
																			arguments().getLiborFixing(),
																			accrualBasis,
																			*(getGlobalComponentCache())));
				} 
				else 
				{
					firstFloatFlowPtr = FixedCashFlow::create(FixedCashFlowArguments(valueDate, 
																 m_scheduleDates.begin()->begin(),
																 m_scheduleDates.begin()->end(),
																 arguments().getLiborFixing(),
																 accrualBasis));
				}

				if( rateDetails.m_depositRateType == DepositRateType::ON || rateDetails.m_depositRateType == DepositRateType::OnArithmetic )
				{
					LTQC_THROW(IDeA::MarketException,"Floating tenor " << tenorDescription << " can not deal with fixings at this time");
				}
			} 
			else 
			{
				switch(rateDetails.m_depositRateType)
				{
					case DepositRateType::IBOR:
						firstFloatFlowPtr = createDiscountedForwardRate(valueDate, iter->begin(), iter->end(), arguments().getPayDelay() , rateDetails.m_accrualValueCalendar, rateBasis, accrualBasis, rateDetails, nbPeriods == periodCnt);
						break;
					case DepositRateType::ON:
						firstFloatFlowPtr = createDiscountedOISCashflow(valueDate, iter->begin(), iter->end(), arguments().getPayDelay() , rateDetails.m_accrualValueCalendar ,rateBasis, accrualBasis, rateDetails.m_currency, rateDetails.m_index );
						break;
					case DepositRateType::OnArithmetic:
						firstFloatFlowPtr = createDiscountedArithmeticOISCashflow(valueDate, iter->begin(), iter->end(), rateDetails.m_accrualValueCalendar, arguments().getPayDelay() , rateDetails.m_accrualValueCalendar ,rateBasis, accrualBasis, rateDetails.m_rateCutOff, rateDetails.m_currency, rateDetails.m_index );
						break;
					case DepositRateType::FUNDING:
						firstFloatFlowPtr = createFundingDiscountedCashflow(valueDate, iter->begin(), iter->end(), rateBasis, rateDetails.m_currency, rateDetails.m_index );
						break;
					default:
						LTQC_THROW(IDeA::MarketException,"FloatingLeg " << tenorDescription << " can not deal with " << rateDetails.m_depositRateType.asString().data() );

				}
			}
			m_cashflows.push_back (firstFloatFlowPtr);

			// process other flows
			
			for(++iter; iter != m_scheduleDates.end(); ++iter)
			{
				++periodCnt;
				switch(rateDetails.m_depositRateType)
				{
					case DepositRateType::IBOR:
						m_cashflows.push_back(createDiscountedForwardRate(valueDate, iter->begin(), iter->end(), arguments().getPayDelay() , rateDetails.m_accrualValueCalendar, rateBasis, accrualBasis, rateDetails, nbPeriods == periodCnt));
						break;
					case DepositRateType::ON:
						m_cashflows.push_back(createDiscountedOISCashflow(valueDate, iter->begin(), iter->end(), arguments().getPayDelay() , rateDetails.m_accrualValueCalendar, rateBasis, accrualBasis,  rateDetails.m_currency, rateDetails.m_index));
						break;
					case DepositRateType::OnArithmetic:
						m_cashflows.push_back(createDiscountedArithmeticOISCashflow(valueDate, iter->begin(), iter->end(), rateDetails.m_accrualValueCalendar, arguments().getPayDelay() , rateDetails.m_accrualValueCalendar, rateBasis, accrualBasis,  rateDetails.m_rateCutOff, rateDetails.m_currency, rateDetails.m_index));
						break;
					case DepositRateType::FUNDING:
						m_cashflows.push_back(createFundingDiscountedCashflow(valueDate, iter->begin(), iter->end(), rateBasis, rateDetails.m_currency, rateDetails.m_index ));
						break;
					default:
						LTQC_THROW(IDeA::MarketException,"FloatingLeg " << tenorDescription << " can not deal with " << rateDetails.m_depositRateType.asString().data() );

				}
			}
		}
	}

	DiscountedForwardRatePtr FloatingLeg::createDiscountedForwardRate(const LT::date valueDate,
																	  const LT::date accrualStartDate,
																	  const LT::date accrualEndDate,
																	  const LTQC::Tenor& payDelay,
																	  const LT::Str& payCalendar,
																	  const DayCounterConstPtr& rateBasis,
																	  const DayCounterConstPtr& accrualBasis,
																	  const IDeA::DepositRateMktConvention& rateDetails,
																	  const bool isLastPeriod) const
	{	
		LT::date payDate(accrualEndDate);	// pay date (assume pay and acc hols are the same)
		
		Date tmp(payDate);
		payDelay.nextPeriod(tmp,payCalendar,LTQC::RollRuleMethod::None);
		payDate = tmp.getAsLTdate();
		// Calculate dates
		LT::date fixingDate, startDate, endDate;
		
		//	calculate index dates, allowing for special end date calculations only for the last period
		if(arguments().getBackStubEndDateCalculationType() != EndDateCalculationType::UseLocal)
		{
			ForwardRate::calculateIndexDates(fixingDate, startDate, endDate, accrualStartDate, accrualEndDate, rateDetails, rateBasis, isLastPeriod? arguments().getBackStubEndDateCalculationType(): rateDetails.m_endDateCalculationType);
		}
		else
		{
			ForwardRate::calculateIndexDates(fixingDate, startDate, endDate, accrualStartDate, accrualEndDate, rateDetails, rateBasis, rateDetails.m_endDateCalculationType);
		}

		return	(
			InstrumentComponent::getUseCacheFlag()	?
			//	Get DiscountedForwardRate object from cache
			getGlobalComponentCache()->get(DiscountedForwardRate::Arguments(valueDate,
																			fixingDate,
																			startDate,
																			endDate,
																			accrualStartDate,
																			accrualEndDate,
																			payDate, 
																		    arguments().getTenorDescription(),
																		    rateBasis,
																			accrualBasis,
                                                                            rateDetails.m_currency,
                                                                            rateDetails.m_index,
																			*(getGlobalComponentCache())))
													:
			//	Always create a new DiscountedForwardRate object
			DiscountedForwardRate::create(DiscountedForwardRateArguments(valueDate,
																		 fixingDate,
																		 startDate,
																		 endDate,
																		 accrualStartDate,
																		 accrualEndDate,
																		 payDate,
																		 arguments().getTenorDescription(),
																		 rateBasis,
																		 accrualBasis,
                                                                         rateDetails.m_currency,
                                                                         rateDetails.m_index))
				);
	}

	DiscountedOISCashflowPtr FloatingLeg::createDiscountedOISCashflow(const LT::date& valueDate,
																	  const LT::date& accrualStartDate,
																	  const LT::date& accrualEndDate,
																	  const LTQC::Tenor& payDelay,
																	  const LT::Str& payCalendar,
																	  const DayCounterConstPtr basisON,
																	  const DayCounterConstPtr accrualBasis,
																	  const LTQC::Currency& ccy, 
                                                                      const LT::Str& index) const
	{	
		return	DiscountedOISCashflow::create(DiscountedOISCashflowArguments(valueDate, accrualStartDate, accrualEndDate, payDelay, payCalendar, basisON, accrualBasis,ccy,index));
	}

    DiscountedArithmeticOISCashflowPtr FloatingLeg::createDiscountedArithmeticOISCashflow(const LT::date& valueDate,
																	  const LT::date& accrualStartDate,
																	  const LT::date& accrualEndDate,
																	  const LT::Str& accCalendar,
																	  const LTQC::Tenor& payDelay,
																	  const LT::Str& payCalendar,
																	  const DayCounterConstPtr basisON,
																	  const DayCounterConstPtr accrualBasis,
																	  size_t cutoff,
																	  const LTQC::Currency& ccy, 
                                                                      const LT::Str& index) const
	{	
		return	DiscountedArithmeticOISCashflow::create(DiscountedArithmeticOISCashflowArguments(valueDate, accrualStartDate, accrualEndDate, accCalendar, payDelay, payCalendar, basisON, accrualBasis,cutoff,ccy,index));
	}
    
    FundingDiscountedCashflowPtr FloatingLeg::createFundingDiscountedCashflow(const LT::date& valueDate,
																	  const LT::date& accrualStartDate,
																	  const LT::date& accrualEndDate,
																	  const DayCounterConstPtr accrualBasis,
																	  const LTQC::Currency& ccy, 
                                                                      const LT::Str& index) const
	{	
		return	FundingDiscountedCashflow::create(FundingDiscountedCashflowArguments(valueDate, accrualStartDate, accrualEndDate, accrualBasis,ccy,index));
	}
	
    void FloatingLeg::fillSingleCashFlowPV(const size_t index,
										   const LT::date startDate,
										   const LT::date endDate,
										   const BaseModel& model,
										   LTQuant::GenericData& cashFlowPVsTable)
	{
		const FloatingLegCashFlowPtr cashFlow(m_cashflows[index]);
		cashFlowPVsTable.set<double>("Rate", index, cashFlow->getRate(model));
		DiscountedForwardRatePtr fwdPtr = std::tr1::dynamic_pointer_cast<DiscountedForwardRate>(cashFlow);
		if (fwdPtr) {
			cashFlowPVsTable.set<double>("Accrual DCF", index, fwdPtr->getArguments().getCoverage());
			cashFlowPVsTable.set<LT::date>("Fixing Start Date", index, fwdPtr->getArguments().getForwardRate()->getFixingDate());
			cashFlowPVsTable.set<LT::date>("Reset Start Date", index, fwdPtr->getArguments().getForwardRate()->getStartDate());
			cashFlowPVsTable.set<LT::date>("Reset End Date", index, fwdPtr->getArguments().getForwardRate()->getEndDate());
			cashFlowPVsTable.set<double>("Reset DCF", index, fwdPtr->getArguments().getForwardRate()->getCoverage());
			cashFlowPVsTable.set<std::string>("Type", index, "Forward");
		} 
        else
        { 
		    DiscountedOISCashflowPtr fwdPtr = std::tr1::dynamic_pointer_cast<DiscountedOISCashflow>(cashFlow);
            if(fwdPtr)
            {
			    cashFlowPVsTable.set<LT::date>("Reset Start Date", index, fwdPtr->getArguments().getStartDate());
			    cashFlowPVsTable.set<LT::date>("Reset End Date", index, fwdPtr->getArguments().getEndDate());
                cashFlowPVsTable.set<LT::date>("Payment Date", index, fwdPtr->getArguments().getPayDate());
			    cashFlowPVsTable.set<std::string>("Type", index, "OIS");  
            }
            else
            {
				DiscountedArithmeticOISCashflowPtr fwdPtr = std::tr1::dynamic_pointer_cast<DiscountedArithmeticOISCashflow>(cashFlow);
				if(fwdPtr)
				{
					cashFlowPVsTable.set<LT::date>("Reset Start Date", index, fwdPtr->getArguments().getStartDate());
					cashFlowPVsTable.set<LT::date>("Reset End Date", index, fwdPtr->getArguments().getEndDate());
					cashFlowPVsTable.set<LT::date>("Payment Date", index, fwdPtr->getArguments().getPayDate());
					cashFlowPVsTable.set<std::string>("Type", index, "OIS Arithmetic");  
				}
				else
				{
					cashFlowPVsTable.set<std::string>("Type", index, "Fixing");
				}
            }
        }
	}

    ostream& FloatingLeg::print(ostream& out) const
    {
        out << "Floating Leg Cash-Flows:" << endl;
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            out << " - ";
            (*iter)->print(out);
        }
        return out;
    }

	std::string FloatingLeg::getLegTypeName() const
	{
		return "Floating Leg";
	}

   /**
        @brief Clone this instance.

        Use a lookup to preserve directed graph relationships.

        @param lookup A lookup of previuos created clones.
        @return       Returns a clone.
    */
    ICloneLookupPtr FloatingLeg::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FloatingLeg(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Use a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    FloatingLeg::FloatingLeg(FloatingLeg const& original, CloneLookup& lookup) : 
        BaseLeg(original)
    {
        CloneLookupUtils::assign(original.m_cashflows, m_cashflows, lookup);
    }

    /// hold on to the schedules, but not the cash flows
    void FloatingLeg::cleanupCashFlows()
    {
        m_cashflows.clear();
    }
}   //  FlexYCF



namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;

	MultiCcyFloatingLeg::MultiCcyFloatingLeg(const FloatingLegArguments& arguments):
		BaseLeg(arguments)
    { 
    }

    double MultiCcyFloatingLeg::getValue(const BaseModel& domModel, const BaseModel& forModel, double fx)
    {
		initializeCashFlowPricing();
        
		double tmpCashFlowSum = 0.0;
        
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            tmpCashFlowSum += (*iter)->getValue(domModel, forModel, fx); 
        }

        return tmpCashFlowSum;
    }

    void MultiCcyFloatingLeg::accumulateGradient(const BaseModel& domModel, const BaseModel& forModel, double fx,
                                                double multiplier,
                                                 GradientIterator gradientBegin,
                                                 GradientIterator gradientEnd)
    {
		initializeCashFlowPricing();
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            (*iter)->accumulateGradient(domModel, forModel, fx, multiplier, gradientBegin, gradientEnd);
        }
    }

	void MultiCcyFloatingLeg::accumulateGradient(const BaseModel& domModel, const BaseModel& forModel, double fx, 
                                         double multiplier,
                                         GradientIterator gradientBegin, 
                                         GradientIterator gradientEnd,
										 const CurveTypeConstPtr& curveType)
	{
		initializeCashFlowPricing();
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            (*iter)->accumulateGradient(domModel, forModel, fx, multiplier, gradientBegin, gradientEnd, curveType);
        }
	}
	
	void MultiCcyFloatingLeg::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                         double multiplier,
										 double fx,
                                         GradientIterator gradientBegin,
                                         GradientIterator gradientEnd,
										 bool spread)
    {
		initializeCashFlowPricing();
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            (*iter)->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier, fx, gradientBegin, gradientEnd, spread);
        }
    }
	
	void MultiCcyFloatingLeg::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                         double multiplier,
										 double fx,
                                         GradientIterator gradientBegin,
                                         GradientIterator gradientEnd,
										 bool spread)
    {
		initializeCashFlowPricing();
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            (*iter)->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier, fx, gradientBegin, gradientEnd, spread);
        }
    }

    void MultiCcyFloatingLeg::update()
    {
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            (*iter)->update();
        }
    }

	

	void MultiCcyFloatingLeg::initializeCashFlows()
	{
		if(m_scheduleDates.empty())
		{
			const string tenorDescription(arguments().getTenorDescription());
	        LTQC::Tenor tenor(tenorDescription == "ON" || tenorDescription == "1D" ? "1M" : tenorDescription);
			
			LTQC::DateBuilderGenerator floatingGen(LTQC::DateBuilder(arguments().getStartDate(), 
																	   arguments().getEndDate(), 
																	   tenor, 
																	   arguments().getCalendarString(), 
																	   arguments().getStubType(),
																	   Date(), 
																	   arguments().getRateDetails().m_rollConvention,    
                                                                       arguments().getRateDetails().m_rollRuleConvention, 
																	   LT::Str("1w"), 
																	   LTQC::RollConv(LTQC::RollConvMethod::None)));	
	       
			 // fill the payment dates
			floatingGen.fillEvents(m_scheduleDates, arguments().getStartDate(), arguments().getEndDate());
		}

	}
	
	void MultiCcyFloatingLeg::doInitializeCashFlowPricing()
	{
		if(m_cashflows.empty())
		{
            const string tenorDescription(arguments().getTenorDescription()); 
            const IDeA::DepositRateMktConvention& rateDetails = arguments().getRateDetails();
	
			const LT::date valueDate(arguments().getValueDate());
			const LT::date fixingDate(arguments().getFixingDate());
			const DayCounterConstPtr rateBasis(LTQC::DayCount::create(rateDetails.m_dcm));
			const DayCounterConstPtr accrualBasis(arguments().getBasis());

		
			Schedule::ScheduleEvents::const_iterator iter(m_scheduleDates.begin());
			const size_t nbPeriods(m_scheduleDates.size());
			size_t periodCnt(1);
			MultiCcyFloatingLegCashFlowPtr firstFloatFlowPtr;

			if (arguments().useLiborFixing()) 
			{
				LTQC_THROW(IDeA::MarketException,"Multi Ccy Floating tenor " << tenorDescription << " can not deal with fixings at this time");
			} 
			else 
			{
				firstFloatFlowPtr = createDiscountedForwardRateNotionalExchange(valueDate, iter->begin(), iter->end(), rateBasis, accrualBasis, rateDetails, nbPeriods == periodCnt);
			}
			m_cashflows.push_back (firstFloatFlowPtr);

			// process other flows
			
			for(++iter; iter != m_scheduleDates.end(); ++iter)
			{
				++periodCnt;
			    m_cashflows.push_back(createDiscountedForwardRateNotionalExchange(valueDate, iter->begin(), iter->end(), rateBasis, accrualBasis, rateDetails, nbPeriods == periodCnt));
			}
		}
	}

	DiscountedForwardRateNotionalExchangePtr MultiCcyFloatingLeg::createDiscountedForwardRateNotionalExchange(const LT::date valueDate,
																	  const LT::date accrualStartDate,
																	  const LT::date accrualEndDate,
																	  const DayCounterConstPtr& rateBasis,
																	  const DayCounterConstPtr& accrualBasis,
																	  const IDeA::DepositRateMktConvention& rateDetails,
																	  const bool isLastPeriod) const
	{	
		const LT::date payDate(accrualEndDate);	// pay date (assume pay and acc hols are the same)

		// Calculate dates
		LT::date fixingDate, startDate, endDate;
		
		//	calculate index dates, allowing for special end date calculations only for the last period
		ForwardRate::calculateIndexDates(fixingDate, startDate, endDate, 
										 accrualStartDate, accrualEndDate,
										 rateDetails, rateBasis,
										 isLastPeriod? arguments().getBackStubEndDateCalculationType(): EndDateCalculationType::NotAdjusted);

		return	
		
			DiscountedForwardRateNotionalExchange::create(DiscountedForwardRateNotionalExchangeArguments(valueDate,
																	     arguments().getSpotFxDate(),
																		 fixingDate,
																		 startDate,
																		 endDate,
																		 accrualStartDate,
																		 accrualEndDate,
																		 payDate,
																		 arguments().getTenorDescription(),
																		 rateBasis,
																		 accrualBasis,
                                                                         rateDetails.m_currency,
                                                                         rateDetails.m_index));
	}

	
	
    void MultiCcyFloatingLeg::fillSingleCashFlowPV(const size_t index,
										   const LT::date startDate,
										   const LT::date endDate,
										   const BaseModel& domModel,
                                           const BaseModel& forModel,
                                           double fx,
										   LTQuant::GenericData& cashFlowPVsTable)
	{
		const MultiCcyFloatingLegCashFlowPtr cashFlow(m_cashflows[index]);
		
		DiscountedForwardRateNotionalExchangePtr fwdPtr = std::tr1::dynamic_pointer_cast<DiscountedForwardRateNotionalExchange>(cashFlow);
		if (fwdPtr) 
        {
            cashFlowPVsTable.set<double>("Rate", index, fwdPtr->getRate(domModel,forModel,fx));
			cashFlowPVsTable.set<double>("Accrual DCF", index, fwdPtr->getArguments().getCoverage());
			cashFlowPVsTable.set<LT::date>("Fwd Fixing", index, fwdPtr->getArguments().getForwardRate()->getFixingDate());
            cashFlowPVsTable.set<double>("Discount Factor Start Date", index, domModel.getDiscountFactor(ModuleDate::getYearsBetween(domModel.getValueDate(), fwdPtr->getArguments().getForwardRate()->getStartDate())));
            cashFlowPVsTable.set<double>("Discount Factor Pay Date", index, domModel.getDiscountFactor(ModuleDate::getYearsBetween(domModel.getValueDate(), fwdPtr->getArguments().getDiscountFactor()->getArguments().getPayDate())));
            cashFlowPVsTable.set<double>("Notional", index, fwdPtr->getNotional(domModel, forModel, fx));
			cashFlowPVsTable.set<std::string>("Type", index, "Forward+NotionalExchange");
		} 
	}

    void MultiCcyFloatingLeg::fillSingleCashFlowPV(const size_t index,
										   const LT::date startDate,
										   const LT::date endDate,
										   const BaseModel& model,
										   LTQuant::GenericData& cashFlowPVsTable)
	{	
	}

    ostream& MultiCcyFloatingLeg::print(ostream& out) const
    {
        out << "Multi Ccy Floating Leg Cash-Flows:" << endl;
        for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
        {
            out << " - ";
            (*iter)->print(out);
        }
        return out;
    }

	std::string MultiCcyFloatingLeg::getLegTypeName() const
	{
		return "Multi Ccy Floating Leg";
	}

  
    ICloneLookupPtr MultiCcyFloatingLeg::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new MultiCcyFloatingLeg(*this, lookup));
    }

    MultiCcyFloatingLeg::MultiCcyFloatingLeg(MultiCcyFloatingLeg const& original, CloneLookup& lookup) : 
        BaseLeg(original)
    {
        CloneLookupUtils::assign(original.m_cashflows, m_cashflows, lookup);
    }

    void MultiCcyFloatingLeg::cleanupCashFlows()
    {
        m_cashflows.clear();
    }

	void MultiCcyFloatingLeg::fillRepFlows(IDeA::AssetDomainConstPtr assetDomainDom,
                                  const BaseModel& domModel,
								  IDeA::AssetDomainConstPtr assetDomainFor,
                                  const BaseModel& forModel,
								  double spot,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		initializeCashFlowPricing();
		for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
		{
			DiscountedForwardRateNotionalExchangePtr fwdPtr = std::tr1::dynamic_pointer_cast<DiscountedForwardRateNotionalExchange>(*iter);
			if(fwdPtr)
				fwdPtr->fillRepFlows(assetDomainDom, domModel, assetDomainFor, forModel, spot, multiplier, fundingRepFlows);
		}
	}

	void MultiCcyFloatingLeg::fillRepFlows(IDeA::AssetDomainConstPtr assetDomainDom,
                                  const BaseModel& domModel,
								  IDeA::AssetDomainConstPtr assetDomainFor,
                                  const BaseModel& forModel,
								  double spot,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		initializeCashFlowPricing();
		for(const_iterator iter(m_cashflows.begin()); iter != m_cashflows.end(); ++iter)
		{
			DiscountedForwardRateNotionalExchangePtr fwdPtr = std::tr1::dynamic_pointer_cast<DiscountedForwardRateNotionalExchange>(*iter);
			if(fwdPtr)
				fwdPtr->fillRepFlows(assetDomainDom, domModel, assetDomainFor, forModel, spot, multiplier, indexRepFlows);
		}
	}
}   //  FlexYCF