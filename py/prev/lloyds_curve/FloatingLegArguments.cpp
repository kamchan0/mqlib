#include "stdafx.h"
#include "FloatingLegArguments.h"
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"
#include "ModuleDate/InternalInterface/CalendarFactory.h"

#include "GlobalComponentCache.h"
#include "DateUtils.h"

//LTQC
#include "DayCount.h"


using namespace LTQC;

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;
   
    FloatingLegArguments::FloatingLegArguments( const LT::date valueDate,
												const LT::date fixingDate,
                                                const LT::date startDate,
                                                const LT::date endDate,
                                                const string& tenorDescription,
					                            const string& accrualCalendar,
                                                const IDeA::DepositRateMktConvention& rateDetails,
												const EndDateCalculationType backStubEndDateCalculationType,
												const LTQC::Tenor& payDelay,
												const DayCounterConstPtr accrualBasis,
                                                const LTQC::StubType& stub):
        m_valueDate(valueDate),
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_tenorDescription(tenorDescription),
        m_useLiborFixing(false),
        m_liborFixing(-10.0e12),        // initialized to a large unrealistic value to help spot error and compare flt leg args
		m_basis( accrualBasis ? accrualBasis : LTQC::DayCount::create(rateDetails.m_dcm)),
		m_calendar(CalendarFactory::create(accrualCalendar)),
		m_calendarString(accrualCalendar),
		m_rateDetails(rateDetails),
		m_backStubEndDateCalculationType(backStubEndDateCalculationType),
		m_payDelay(payDelay),
        m_stubType(stub),
		m_spotFxDate()
    {
        LTQC::Tenor tenor(tenorDescription); 
        if( tenor.getWeeks() + tenor.getDays() > 0 )
        {
            m_rateDetails.m_rollRuleConvention = LTQC::RollRuleMethod(LTQC::RollRuleMethod::None);
        }
    }

		FloatingLegArguments::FloatingLegArguments( const LT::date valueDate,
												const LT::date spotFxDate,
												const LT::date fixingDate,
                                                const LT::date startDate,
                                                const LT::date endDate,
                                                const string& tenorDescription,
					                            const string& accrualCalendar,
                                                const IDeA::DepositRateMktConvention& rateDetails,
												const EndDateCalculationType backStubEndDateCalculationType,
												const LTQC::Tenor& payDelay,
												const DayCounterConstPtr accrualBasis,
                                                const LTQC::StubType& stub):
        m_valueDate(valueDate),
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_tenorDescription(tenorDescription),
        m_useLiborFixing(false),
        m_liborFixing(-10.0e12),        // initialized to a large unrealistic value to help spot error and compare flt leg args
		m_basis( accrualBasis ? accrualBasis : LTQC::DayCount::create(rateDetails.m_dcm)),
		m_calendar(CalendarFactory::create(accrualCalendar)),
		m_calendarString(accrualCalendar),
		m_rateDetails(rateDetails),
		m_backStubEndDateCalculationType(backStubEndDateCalculationType),
		m_payDelay(payDelay),
        m_stubType(stub),
		m_spotFxDate(spotFxDate)
    {
        LTQC::Tenor tenor(tenorDescription); 
        if( tenor.getWeeks() + tenor.getDays() > 0 )
        {
            m_rateDetails.m_rollRuleConvention = LTQC::RollRuleMethod(LTQC::RollRuleMethod::None);
        }
    }

    FloatingLegArguments::FloatingLegArguments( const LT::date valueDate,
                                                const LT::date fixingDate,
                                                const LT::date startDate,
                                                const LT::date endDate,
                                                const string& tenorDescription,
 					                            const string& accrualCalendar,
                                                const IDeA::DepositRateMktConvention& rateDetails,
												const double liborFixing,
												const EndDateCalculationType backStubEndDateCalculationType,
												const LTQC::Tenor& payDelay,
												const DayCounterConstPtr accrualBasis,
                                                const LTQC::StubType& stub):
        m_valueDate(valueDate),
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_tenorDescription(tenorDescription),
        m_useLiborFixing(true),
        m_liborFixing(liborFixing),
		m_basis( accrualBasis ? accrualBasis : LTQC::DayCount::create(rateDetails.m_dcm)),
		m_calendar(CalendarFactory::create(accrualCalendar)),
		m_calendarString(accrualCalendar),
		m_rateDetails(rateDetails),
		m_backStubEndDateCalculationType(backStubEndDateCalculationType),
		m_payDelay(payDelay),
        m_stubType(stub),
		m_spotFxDate()
    {
        LTQC::Tenor tenor(tenorDescription); 
        if( tenor.getWeeks() + tenor.getDays() > 0 )
        {
            m_rateDetails.m_rollRuleConvention = LTQC::RollRuleMethod(LTQC::RollRuleMethod::None);
        }
        //  Note: The calculations of the reset and payment dates according to basis / calendar
        //          are done at FloatingLeg::initialize level

        //  std::cout << "FltLegArgs, w/ Fix" << std::endl;
    }

}   //  FlexYCF
