#include "stdafx.h"
#include "FixedLegArguments.h"
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"
#include "ModuleDate/InternalInterface/Calendar.h"
#include "DateUtils.h"

#include "ModuleDate/InternalInterface/CalendarFactory.h"
#include "ModuleDate/InternalInterface/DayCounterFactory.h"
#include "GlobalComponentCache.h"


using namespace LTQC;

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;

    FixedLegArguments::FixedLegArguments( const LT::date valueDate,
                                          const LT::date fixingDate,
                                          const LT::date startDate,
                                          const LT::date endDate,
                                          const string& tenorDescription,
                                          const string& basisName,
										  const string& calName,
                                          const LT::Str& currency,
                                          const LT::Str& market,
										  const LTQC::RollConvMethod& rollConv,
										  const LTQC::RollRuleMethod& rollRule,
										  const LT::Str& payDelay,
										  const LT::Str& payCalendar,
										  const LT::Str& payRollConv,
                                          const LTQC::StubType& stub):
        m_valueDate(valueDate),
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_tenorDescription(tenorDescription),
        m_basis(DayCounterFactory::create(basisName.c_str())),
		m_calendar(CalendarFactory::create(calName)),
		m_calendarString(calName),
        m_currency(currency),
        m_market(market),
		m_rollConvention(rollConv),
		m_rollRuleConvention(rollRule),
		m_payDelay(payDelay),
		m_payCalendar(payCalendar),
		m_payRollConvention(payRollConv),
        m_stubType(stub)
        
    {
        LTQC::Tenor tenor(tenorDescription); 
        if( tenor.getWeeks() + tenor.getDays() > 0 )
        {
            m_rollRuleConvention = LTQC::RollRuleMethod(LTQC::RollRuleMethod::None);
        }
    }

}   //  FlexYCF