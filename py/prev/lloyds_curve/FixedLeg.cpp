/*****************************************************************************
    
	FixedLeg

	Implementation of the FixedLeg class.

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

// IDeA
#include "AssetDomain.h"

//	FlexYCF
#include "FixedLeg.h"
#include "DiscountFactor.h"
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "GlobalComponentCache.h"
#include "RepFlowsData.h"
#include "FlexYCFCloneLookup.h"

#include "RollConv.h"
#include "dates/DateBuilderGenerator.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    double FixedLeg::getValue(BaseModel const& baseModel)
    {
		initializeCashFlowPricing();
        
		double tmpCashFlowSum = 0.0;

        for(const_iterator iter(m_cashFlows.begin()); iter != m_cashFlows.end(); ++iter)
        {
            tmpCashFlowSum += iter->first * iter->second->getValue(baseModel);  // cvg * df
        }

        return tmpCashFlowSum;
    }

    vector< pair<LT::date, double> > FixedLeg::getAnnuity(BaseModel const& baseModel)
    {
		initializeCashFlowPricing();
        vector< pair<LT::date, double> > annuity(m_cashFlows.size()+1);
        annuity[0].second = 0.0;
        annuity[0].first  = getValueDate();
        
        size_t i = 1;
        for(const_iterator iter(m_cashFlows.begin()); iter != m_cashFlows.end(); ++iter, ++i)
        {
            annuity[i].second = annuity[i-1].second + iter->first * iter->second->getValue(baseModel);  // cvg * df
            annuity[i].first  = iter->second->getArguments().getPayDate();
        }

        return annuity;
    }

    // The gradient of a fixed leg is computed by linearity, a fixed leg being a linear combination
    //  of discount factors
    void FixedLeg::accumulateGradient(BaseModel const& baseModel,
                                      double multiplier,
                                      GradientIterator gradientBegin,
                                      GradientIterator gradientEnd)
    {
		initializeCashFlowPricing();
        
        for(const_iterator iter(m_cashFlows.begin()); iter != m_cashFlows.end(); ++iter)
        {
            // compute the gradient for this discount factor:
            // multiply it by its related coverage:
            iter->second->accumulateGradient(baseModel, multiplier * iter->first, gradientBegin, gradientEnd);
        }
    }

	void FixedLeg::accumulateGradient(BaseModel const& baseModel, 
                                      double multiplier, 
                                      GradientIterator gradientBegin, 
                                      GradientIterator gradientEnd,
									  const CurveTypeConstPtr& curveType)
	{
		initializeCashFlowPricing();
        
		for(const_iterator iter(m_cashFlows.begin()); iter != m_cashFlows.end(); ++iter)
        {
            // compute the gradient for this discount factor:
            // multiply it by its related coverage:
            iter->second->accumulateGradient(baseModel, multiplier * iter->first, gradientBegin, gradientEnd, curveType);
        }
	}

    void FixedLeg::update()
    {
        for(const_iterator iter(m_cashFlows.begin()); iter != m_cashFlows.end(); ++iter)
        {
            iter->second->update();
        }
    }

	void FixedLeg::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                const BaseModel& model,
								const double multiplier,
								IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		initializeCashFlowPricing();

		for(const_iterator iter(m_cashFlows.begin()); iter != m_cashFlows.end(); ++iter)
		{
			//	add (df date, coverage * multiplier) pair 
			fundingRepFlows.addRepFlow(IDeA::Funding::Key(model.primaryAssetDomainFunding(), iter->second->getArguments().getPayDate()), multiplier * iter->first);
		}
	}


	void FixedLeg::initializeCashFlows()
	{
		using namespace LTQuant;
       
		// if m_cvg is provided, ignore m_scheduleDates (for Bond instrument)

		if (!m_cvg.empty())
		{
			if (m_cvg.size() != m_paymentDates.size())
				LTQC_THROW(IDeA::MarketException, "FixedLeg::initializeCashFlows - the number of year fraction is inconsistent with that of payDates");
		}

		if(m_scheduleDates.empty() && m_cvg.empty())
		{
			const string tenorDescription(arguments().getTenorDescription());

			// ON & 1D are NOT handled by ScheduleGenerator,
			//  so they are interpreted as 1M for now.
	        LTQC::Tenor tenor(tenorDescription == "ON" || tenorDescription == "1D" ? "1M" : tenorDescription);

			LTQC::DateBuilderGenerator fixedGen(LTQC::DateBuilder(arguments().getStartDate(), 
																	   arguments().getEndDate(), 
																	   tenor, 
																	   arguments().getCalendarString(), 
																	   arguments().getStubType(),
																	   Date(), 
																	   arguments().getRollConvMethod(),    
																	   arguments().getRollRuleMethod(), 
																	   LT::Str("1w"), 
																	   LTQC::RollConv(LTQC::RollConvMethod::None),
																	   arguments().payDelay(),
																	   arguments().payCalendar(),
																	   arguments().payRollConvention()));	
			// fill the payment dates
			fixedGen.fillEvents(m_scheduleDates, m_paymentDates, arguments().getStartDate(), arguments().getEndDate());
		}

	}

	void FixedLeg::doInitializeCashFlowPricing()
	{
		if(m_cashFlows.empty())
		{
			const LT::date valueDate(arguments().getValueDate());
			const ModuleDate::DayCounterConstPtr basis(arguments().getBasis());
            const LT::Str& ccy = arguments().currency();
            const LT::Str& mkt = arguments().market();
			// fill the cash-flows
			vector<Date>::const_iterator itPayment = m_paymentDates.begin();
			if (m_cvg.empty())
			{
				for(ModuleDate::Schedule::ScheduleEvents::const_iterator iter(m_scheduleDates.begin()); iter != m_scheduleDates.end(); ++iter, ++itPayment)
				{
					const LT::date& paymentDate = (*itPayment).getAsLTdate();
					m_cashFlows.push_back  (
						CvgDfPair(	basis->getDaysOverBasis(iter->begin(), iter->end()), 
										(InstrumentComponent::getUseCacheFlag()
										? getGlobalComponentCache()->get(DiscountFactor::Arguments(valueDate, paymentDate, ccy, mkt))
										: DiscountFactor::create(DiscountFactorArguments(valueDate, paymentDate, ccy, mkt)))
								)   
					);
				}
			}
			else	// if m_cvg is provided, ignore m_scheduleDates
			{
				for(std::vector<double>::const_iterator iter(m_cvg.begin()); iter != m_cvg.end(); ++iter, ++itPayment)
				{
					const LT::date& paymentDate = (*itPayment).getAsLTdate();
					m_cashFlows.push_back  (
						CvgDfPair	( *iter, (InstrumentComponent::getUseCacheFlag()
												? getGlobalComponentCache()->get(DiscountFactor::Arguments(valueDate, paymentDate, ccy, mkt))
												: DiscountFactor::create(DiscountFactorArguments(valueDate, paymentDate, ccy, mkt)))
									)   
					);
				}
			}

		}
	}

	void FixedLeg::fillSingleCashFlowPV(const size_t index,
										const LT::date startDate,
										const LT::date endDate,
										const BaseModel& model,
										LTQuant::GenericData& cashFlowPVsTable)
	{
		CvgDfPair cashFlow(m_cashFlows[index]);
		cashFlowPVsTable.set<double>("Accrual DCF", index, cashFlow.first);
        cashFlowPVsTable.set<LT::date>("Payment Date", index, m_paymentDates[index].getAsLTdate());
        cashFlowPVsTable.set<double>("Discount Factor", index, model.getDiscountFactor(ModuleDate::getYearsBetween(model.getValueDate(), m_paymentDates[index].getAsLTdate())));
	}
    
    ostream& FixedLeg::print(ostream& out) const
    {
        out << "Fxd Leg" << endl;
        // TO DO: output cash-flows
        return out;
    }
	
	std::string FixedLeg::getLegTypeName() const
	{
		return "Fixed Leg";
	}

   /**
        @brief Clone this instance.

        Use a lookup to preserve directed graph relationships.

        @param lookup A lookup of previuos created clones.
        @return       Returns a clone.
    */
    ICloneLookupPtr FixedLeg::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FixedLeg(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Use a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    FixedLeg::FixedLeg(FixedLeg const& original, CloneLookup& lookup) : 
        BaseLeg(original)
    {
        // Copy across the discount factors, cloning as we go
        m_cashFlows.reserve(original.m_cashFlows.size());
        for (CvgDfContainer::const_iterator it = original.m_cashFlows.begin(); it != original.m_cashFlows.end(); ++it)
            m_cashFlows.push_back(CvgDfPair((*it).first, lookup.get((*it).second)));
    }

    /// hold on to the schedules, but not the cash flows
    void FixedLeg::cleanupCashFlows()
    {
        m_cashFlows.clear();
    }

    

}   //  FlexYCF
