#include "stdafx.h"
#include "DiscountedOISCashflowArguments.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "DateUtils.h"
#include "GlobalComponentCache.h"

using namespace std;


using namespace LTQC;

namespace FlexYCF
{
    DiscountedOISCashflowArguments::DiscountedOISCashflowArguments(const LT::date& valueDate,
															   const LT::date& startDate,
															   const LT::date& endDate,
															   const LTQC::Tenor& payDelay,
															   const LT::Str& payCalendar,
															   const ModuleDate::DayCounterConstPtr basisON,
															   const ModuleDate::DayCounterConstPtr basisCashflow,
                                                               const LTQC::Currency& ccy, 
                                                               const LT::Str& index):
        m_startDate(startDate), 
		m_endDate(endDate), 
		m_valueDate(valueDate),
		m_payDate(endDate),
		m_coverageON(basisON->getDaysOverBasis(startDate, endDate)),
		m_coverage(basisCashflow->getDaysOverBasis(startDate, endDate)),
        m_ccy(ccy),
        m_index(index)
	{
		Date payDate(endDate);
		payDelay.nextPeriod(payDate,payCalendar,LTQC::RollRuleMethod::None);
		m_payDate = payDate.getAsLTdate();
	}
   
    double DiscountedOISCashflowArguments::getFlowTime() const
    {
        return ModuleDate::getYearsBetween(m_valueDate, m_payDate);
    }

    LT::date DiscountedOISCashflowArguments::getPayDate() const
    {
        return m_payDate;
    }

    ostream& DiscountedOISCashflowArguments::print(ostream& out) const
    {
        out << "flow time = " << getFlowTime();
        return out;
    }

	DiscountedOISCashflowArguments::DiscountedOISCashflowArguments(DiscountedOISCashflowArguments const& original, CloneLookup& lookup) 
		: m_startDate(original.m_startDate), m_endDate(original.m_endDate), m_valueDate(original.m_valueDate), m_payDate(original.m_payDate), m_coverageON(original.m_coverageON),  m_coverage(original.m_coverage), m_ccy(original.m_ccy), m_index(original.m_index)  {}
}

namespace FlexYCF
{
    FundingDiscountedCashflowArguments::FundingDiscountedCashflowArguments(const LT::date& valueDate,
															   const LT::date& startDate,
															   const LT::date& endDate,
															   const ModuleDate::DayCounterConstPtr basisCashflow,
                                                               const LTQC::Currency& ccy, 
                                                               const LT::Str& index):
        m_startDate(startDate), 
		m_endDate(endDate), 
		m_valueDate(valueDate),
		m_coverage(basisCashflow->getDaysOverBasis(startDate, endDate)),
        m_ccy(ccy),
        m_index(index)
	{
	}
   
    double FundingDiscountedCashflowArguments::getFlowTime() const
    {
        return ModuleDate::getYearsBetween(m_valueDate, m_endDate);
    }

    LT::date FundingDiscountedCashflowArguments::getPayDate() const
    {
        return m_endDate;
    }

    ostream& FundingDiscountedCashflowArguments::print(ostream& out) const
    {
        out << "flow time = " << getFlowTime();
        return out;
    }

	FundingDiscountedCashflowArguments::FundingDiscountedCashflowArguments(FundingDiscountedCashflowArguments const& original, CloneLookup& lookup) 
		: m_startDate(original.m_startDate), m_endDate(original.m_endDate), m_valueDate(original.m_valueDate), m_coverage(original.m_coverage), m_ccy(original.m_ccy), m_index(original.m_index) {}
}


namespace FlexYCF
{
    DiscountedArithmeticOISCashflowArguments::DiscountedArithmeticOISCashflowArguments(const LT::date& valueDate,
															   const LT::date& startDate,
															   const LT::date& endDate,
															   const LT::Str& accrualCalendar,
															   const LTQC::Tenor& payDelay,
															   const LT::Str& payCalendar,
															   const ModuleDate::DayCounterConstPtr basisON,
															   const ModuleDate::DayCounterConstPtr basisCashflow,
															   size_t cutoff,
                                                               const LTQC::Currency& ccy, 
                                                               const LT::Str& index):
        m_startDate(startDate), 
		m_endDate(endDate), 
		m_valueDate(valueDate),
		m_payDate(endDate),
		m_cutoff(cutoff),
		m_accrualCalendar(accrualCalendar),
		m_cutOffAdj(1.0),
		m_coverageON(basisON->getDaysOverBasis(startDate, endDate)),
		m_coverage(basisCashflow->getDaysOverBasis(startDate, endDate)),
        m_ccy(ccy),
        m_index(index)
	{
		Date payDate(endDate);
		payDelay.nextPeriod(payDate,payCalendar,LTQC::RollRuleMethod::None);
		m_payDate = payDate.getAsLTdate();
		
		LTQC::Tenor accrualPeriod(0,1,0,0,0);
		LT::date tmp(startDate);
		while(tmp < endDate)
		{
			m_endDates.push_back(tmp);
			Date tmp2(tmp);
			accrualPeriod.nextPeriod(tmp2, m_accrualCalendar, LTQC::RollRuleMethod::None);
			tmp = tmp2.getAsLTdate();
		}
		m_endDates.push_back(endDate);

		if(m_endDates.size() < 2)
		{
			LTQC_THROW(IDeA::MarketException,"Can not find dates schedule for OIS cashflow with arithmetic average");
		}
		if(m_cutoff < 1)
		{
			LTQC_THROW(IDeA::MarketException,"OIS rate cutoff has to be positive");
		}
		
		
		if(m_cutoff != 1)
		{
			// use only the first rate
			if(m_cutoff >= m_endDates.size() - 1)
			{
				LT::date startON = m_endDates[0];
				double adj = 0.0;
			
				for(size_t i=1; i<m_endDates.size(); ++i)
				{
					LT::date endON = m_endDates[i];
					adj += basisON->getDaysOverBasis(startON, endON);
					startON = endON;
				}

				m_cutOffAdj = adj / basisON->getDaysOverBasis(m_endDates[0], m_endDates[1]);
				auto it = m_endDates.begin(), itEnd = m_endDates.end();
				std::advance(it,2);
				m_endDates.erase(it,itEnd);
			}
			else
			{
				size_t k = m_endDates.size() - 1;
				size_t i = m_cutoff;
				double adj = 0.0;
				LT::date endON = m_endDates[k];
				--k;
				while(i > 0)
				{
					LT::date startON = m_endDates[k];
					adj += basisON->getDaysOverBasis(startON, endON);
					endON = startON;

					--i;
					--k;
				}
				
				m_cutOffAdj = adj / basisON->getDaysOverBasis(m_endDates[k+1], m_endDates[k+2]);
				auto it = m_endDates.begin(), itEnd = m_endDates.end();
				std::advance(it, m_endDates.size() + 1 - m_cutoff);
				m_endDates.erase(it,itEnd);
			}
		}
	}
   
    double DiscountedArithmeticOISCashflowArguments::getFlowTime() const
    {
        return ModuleDate::getYearsBetween(m_valueDate, m_payDate);
    }

    LT::date DiscountedArithmeticOISCashflowArguments::getPayDate() const
    {
        return m_payDate;
    }

    ostream& DiscountedArithmeticOISCashflowArguments::print(ostream& out) const
    {
        out << "flow time = " << getFlowTime();
        return out;
    }

	DiscountedArithmeticOISCashflowArguments::DiscountedArithmeticOISCashflowArguments(DiscountedArithmeticOISCashflowArguments const& original, CloneLookup& lookup) 
		: m_startDate(original.m_startDate), m_endDate(original.m_endDate), m_valueDate(original.m_valueDate), m_payDate(original.m_payDate), m_accrualCalendar(original.m_accrualCalendar), m_cutOffAdj(original.m_cutOffAdj), m_coverageON(original.m_coverageON),  m_coverage(original.m_coverage), m_ccy(original.m_ccy), m_index(original.m_index)  
	{
		for(size_t i=0; i<original.m_endDates.size(); ++i)
		{
			m_endDates.push_back(original.m_endDates[i]);
		}
	}
}