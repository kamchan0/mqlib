/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FIXEDLEGARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FIXEDLEGARGUMENTS_H_INCLUDED
#pragma once
#include "DiscountFactor.h"
#include "ModuleDate/InternalInterface/DayCounter.h"

#include "IDeA\src\market\MarketConvention.h"

namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
    FWD_DECLARE_SMART_PTRS( Calendar )
}


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( InstrumentComponent )

    class GlobalComponentCache;

   
    /// FixedLegArguments encapsulates the parameters required
    /// to create a fixed leg component.
    ///
    /// Implementation Note:
    /// A FixedLeg component is internally represented as a vector of 
    /// pairs of (coverage, discount factors). The coverages are
    /// constant and computed once and for all in the constructor. The discount
    /// factors are retrieved using the global component cache. Their getValue()
    /// computeGradient functions can be cached and cleared.
    class FixedLegArguments
    {
    public:
    
        FixedLegArguments(const LT::date valueDate,
                          const LT::date fixingDate,
                          const LT::date startDate,
                          const LT::date endDate,
                          const string& tenorDescription,
                          const string& basisName,
                          const string& calendarName,
                          const LT::Str& currency = "",
                          const LT::Str& market = "",
						  const LTQC::RollConvMethod& rollConv = LTQC::RollConvMethod::ModifiedFollowing,
						  const LTQC::RollRuleMethod& rollRule = LTQC::RollRuleMethod::BusinessEOM,
						  const LT::Str& payDelay    = "0B",
						  const LT::Str& payCalendar = "",
						  const LT::Str& payRollConv = "Next",
                          const LTQC::StubType& stub =  LTQC::StubType::SE);

        /// Comparison functor and equality function implementation
        ///  was modified so as to compare FixedLegArguments 
        ///  without generating the cash-flows which are now created
        ///  at the FixedLeg level
        struct Compare
        {
            bool operator()(const FixedLegArguments& lhs, const FixedLegArguments& rhs)
            {   
                if(lhs.m_tenorDescription != rhs.m_tenorDescription)
                {
                    return (lhs.m_tenorDescription < rhs.m_tenorDescription);
                }
                if(lhs.m_currency.compareCaseless( rhs.m_currency ) != 0)
                {
                    return lhs.m_currency.string() < rhs.m_currency.string();
                }
                if(lhs.m_market.compareCaseless( rhs.m_market ) != 0)
                {
                    return lhs.m_market.string() < rhs.m_market.string();
                }
				if(lhs.m_basis->getName() != rhs.m_basis->getName())
                {
                    return (lhs.m_basis->getName() < rhs.m_basis->getName());
                }
                if(lhs.m_endDate != rhs.m_endDate)
                {
                    return lhs.m_endDate < rhs.m_endDate;
                }
                if(lhs.m_startDate != rhs.m_startDate)
                {
                    return lhs.m_startDate < rhs.m_startDate;
                }
				if(lhs.m_payDelay != rhs.m_payDelay)
                {
                    return lhs.m_payDelay.string() < rhs.m_payDelay.string();
                }
				if(lhs.m_payCalendar != rhs.m_payCalendar)
                {
                    return lhs.m_payCalendar.string() < rhs.m_payCalendar.string();
                }
				if(lhs.m_payRollConvention != rhs.m_payRollConvention)
                {
                    return lhs.m_payRollConvention.string() < rhs.m_payRollConvention.string();
                }
                if(lhs.m_calendarString != rhs.m_calendarString)
                {
                    return lhs.m_calendarString < rhs.m_calendarString;
                }
				if(lhs.m_stubType.asString() != rhs.m_stubType.asString())
                {
                    return lhs.m_stubType.asString() < rhs.m_stubType.asString();
                }
                return false;
            }
        };

        // used by CalculationCache -- NOT VALID AS IT IS
        bool operator==(const FixedLegArguments& /* other */) const
        {
            /*
            if(m_cvgDfs.size() != other.m_cvgDfs.size())
            {
                return false;
            }

            for(size_t flow(0); flow < m_cvgDfs.size(); ++flow)
            {
                if(m_cvgDfs[flow].first != other.m_cvgDfs[flow].first
                    || !(m_cvgDfs[flow].second->operator==(*(other.m_cvgDfs[flow].second))))
                {
                    return false;
                }
            }*/
            return true;
        }

      
        LT::date getFixingDate()          const  { return m_fixingDate; }
        LT::date getValueDate()          const  { return m_valueDate; }
        LT::date getStartDate()          const  { return m_startDate; }
        LT::date getEndDate()            const  { return m_endDate; }
        string getTenorDescription() const  { return m_tenorDescription; }
        ModuleDate::DayCounterConstPtr getBasis() const { return m_basis; }
        ModuleDate::CalendarPtr getCalendar() const { return m_calendar; }
		string getCalendarString()         const { return m_calendarString; }
		LTQC::RollConvMethod getRollConvMethod() const {return m_rollConvention; }
		LTQC::RollRuleMethod getRollRuleMethod() const {return m_rollRuleConvention; }

		LT::Str payDelay() const { return m_payDelay;}
		LT::Str payCalendar() const { return m_payCalendar;}
		LT::Str payRollConvention() const { return m_payRollConvention;}
        LTQC::StubType  getStubType() const { return  m_stubType; }
	    LT::Str currency() const { return m_currency;}
        LT::Str market() const { return m_market;}

    private:
        LT::date m_valueDate;
        LT::date m_fixingDate;
        LT::date m_startDate;
        LT::date m_endDate;
        string m_tenorDescription;
        ModuleDate::DayCounterConstPtr m_basis;
        ModuleDate::CalendarPtr m_calendar;
        LT::Str                 m_currency;
        LT::Str                 m_market;

		string	m_calendarString;
		LTQC::RollConvMethod	m_rollConvention; //Following/Modified Following, etc
		LTQC::RollRuleMethod	m_rollRuleConvention; //BusinessEOM, etc
		
		LT::Str					m_payDelay;
		LT::Str				    m_payCalendar;
		LT::Str					m_payRollConvention;

        LTQC::StubType          m_stubType;   
    };  //  FixedLegArguments

	   
	size_t hash_value(const std::pair<double, DiscountFactorPtr>& cvgDfPair);

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_FIXEDLEGARGUMENTS_H_INCLUDED