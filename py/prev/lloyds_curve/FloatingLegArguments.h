/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FLOATINGLEGARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FLOATINGLEGARGUMENTS_H_INCLUDED
#pragma once

//	FlexYCF
#include "DiscountedForwardRate.h"

//	QuantLib
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "ModuleStaticData/InternalInterface/IRIndexProperties.h"
#include "StubUtils.h"
#include "tenor.h"

//	IDeA
#include "IDeA\src\market\MarketConvention.h"


namespace ModuleDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
    FWD_DECLARE_SMART_PTRS( Calendar )
}

namespace FlexYCF
{
    class GlobalComponentCache;

    /// FloatingLegArguments encapsulates the parameters 
    /// required to create a floating leg component.
    class FloatingLegArguments
    {
    public:
        FloatingLegArguments(const LT::date valueDate, 
							 const LT::date fixingDate,
                             const LT::date startDate,
                             const LT::date endDate,
                             const string& tenorDescription,
                             const string& accrualCalendar,
                             const IDeA::DepositRateMktConvention& rateDetails,
							 const EndDateCalculationType backStubEndDateCalculationType = EndDateCalculationType::NotAdjusted,
							 const LTQC::Tenor& payDelay = LTQC::Tenor(),
							 const ModuleDate::DayCounterConstPtr accrualBasis = ModuleDate::DayCounterConstPtr(),
                             const LTQC::StubType& stub =  LTQC::StubType::SE);

		FloatingLegArguments(const LT::date valueDate,
							 const LT::date spotFxDate,
							 const LT::date fixingDate,
                             const LT::date startDate,
                             const LT::date endDate,
                             const string& tenorDescription,
                             const string& accrualCalendar,
                             const IDeA::DepositRateMktConvention& rateDetails,
							 const EndDateCalculationType backStubEndDateCalculationType = EndDateCalculationType::NotAdjusted,
							 const LTQC::Tenor& payDelay = LTQC::Tenor(),
							 const ModuleDate::DayCounterConstPtr accrualBasis = ModuleDate::DayCounterConstPtr(),
                             const LTQC::StubType& stub =  LTQC::StubType::SE);

        // New ctor to handle LIBOR Fixing
        FloatingLegArguments(const LT::date valueDate,
							 const LT::date fixingDate,
                             const LT::date startDate,
                             const LT::date endDate,
                             const string& tenorDescription,
                             const string& accrualCalendar,
                             const IDeA::DepositRateMktConvention& rateDetails,
							 const double liborFixing,
							 const EndDateCalculationType backStubEndDateCalculationType = EndDateCalculationType::NotAdjusted,
							 const LTQC::Tenor& payDelay = LTQC::Tenor(),
							 const ModuleDate::DayCounterConstPtr accrualBasis = ModuleDate::DayCounterConstPtr(),
                             const LTQC::StubType& stub =  LTQC::StubType::SE);
        
        struct Compare
        {
            bool operator()(const FloatingLegArguments& lhs, const FloatingLegArguments& rhs)
            {
                if(lhs.m_liborFixing != rhs.m_liborFixing)
                {
                    return lhs.m_liborFixing < rhs.m_liborFixing;
                }
				if(lhs.m_useLiborFixing != rhs.m_useLiborFixing)
                {
                    return lhs.m_useLiborFixing < rhs.m_useLiborFixing;
                }
                if(lhs.m_tenorDescription != rhs.m_tenorDescription)
                {
                    return lhs.m_tenorDescription < rhs.m_tenorDescription;
                }
                if(lhs.m_basis->getName() != rhs.m_basis->getName())
                {
                    return lhs.m_basis->getName() < rhs.m_basis->getName();
                }
                if(lhs.m_endDate != rhs.m_endDate)
                {
                    return lhs.m_endDate < rhs.m_endDate;
                }
                if(lhs.m_startDate != rhs.m_startDate)
                {
                    return lhs.m_startDate < rhs.m_startDate;
                }
				if(lhs.m_fixingDate != rhs.m_fixingDate)
                {
                    return lhs.m_fixingDate < rhs.m_fixingDate;
                }
				if(lhs.m_valueDate != rhs.m_valueDate)
                {
                    return lhs.m_valueDate < rhs.m_valueDate;
                }

				if(lhs.m_payDelay.asString() != rhs.m_payDelay.asString())
                {
                    return lhs.m_payDelay.asString() < rhs.m_payDelay.asString();
                }
				if(lhs.m_calendarString != rhs.m_calendarString)
                {
                    return lhs.m_calendarString < rhs.m_calendarString;
                }
				if(lhs.m_stubType.asString() != rhs.m_stubType.asString())
                {
                    return lhs.m_stubType.asString() < rhs.m_stubType.asString();
                }
				if(lhs.m_backStubEndDateCalculationType.asString() != rhs.m_backStubEndDateCalculationType.asString())
                {
                    return lhs.m_backStubEndDateCalculationType.asString() < rhs.m_backStubEndDateCalculationType.asString();
                }
				if(lhs.m_rateDetails.m_dcm.asString() != rhs.m_rateDetails.m_dcm.asString())
                {
                    return lhs.m_rateDetails.m_dcm.asString() < rhs.m_rateDetails.m_dcm.asString();
                }
				if(lhs.m_spotFxDate != rhs.m_spotFxDate)
                {
                    return lhs.m_spotFxDate < rhs.m_spotFxDate;
                }
				LTQC::Tenor tenor(lhs.m_tenorDescription); 
				if( tenor.getWeeks() + tenor.getDays() == 0 )
				{
					if(lhs.m_rateDetails.m_rollRuleConvention.asString() != rhs.m_rateDetails.m_rollRuleConvention.asString())
					{
						return lhs.m_rateDetails.m_rollRuleConvention.asString() < rhs.m_rateDetails.m_rollRuleConvention.asString();
					}
				}
				return IDeA::DepositRateMktConvention::Compare()(lhs.m_rateDetails,rhs.m_rateDetails);

            }
        };

        // used by CalculationCache -- NOT VALID AS IT IS
        bool operator==(const FloatingLegArguments& /* other */) const
        {
            /*
            if(m_dfrs.size() != other.m_dfrs.size())
            {
                return false;
            }
            for(size_t flow(0); flow < m_dfrs.size(); ++flow)
            {
                if(!m_dfrs[flow]->operator==(*(other.m_dfrs[flow])))
                {
                    return false;
                }
            }*/
            return true;
        }

        inline LT::date									getFixingDate() const						{ return m_fixingDate;						}
        inline LT::date									getValueDate() const						{ return m_valueDate;						}
        inline LT::date									getStartDate() const						{ return m_startDate;						}
        inline LT::date									getEndDate() const							{ return m_endDate;							}
        inline string									getTenorDescription() const					{ return m_tenorDescription;				}
        inline bool										useLiborFixing() const						{ return m_useLiborFixing;					}
        inline double									getLiborFixing() const						{ return m_liborFixing;						}
        inline ModuleDate::DayCounterConstPtr			getBasis() const							{ return m_basis;							}
        inline ModuleDate::CalendarPtr					getCalendar() const							{ return m_calendar;						}
		inline string									getCalendarString() const					{ return m_calendarString;					}
        inline const IDeA::DepositRateMktConvention&	getRateDetails() const						{ return m_rateDetails;						}
		inline EndDateCalculationType					getBackStubEndDateCalculationType() const	{ return m_backStubEndDateCalculationType;	}
		const LTQC::Tenor&								getPayDelay() const							{ return m_payDelay; }
        LTQC::StubType                                  getStubType() const                         { return m_stubType; }
		inline LT::date									getSpotFxDate() const						{ return m_spotFxDate;						}
    private:
        LT::date						m_fixingDate;
        LT::date						m_valueDate;
        LT::date						m_startDate;
        LT::date						m_endDate;
        string							m_tenorDescription;
        bool							m_useLiborFixing;
        double							m_liborFixing;
        ModuleDate::DayCounterConstPtr	m_basis;
        ModuleDate::CalendarPtr			m_calendar;
		string						    m_calendarString;
        IDeA::DepositRateMktConvention	m_rateDetails;
		EndDateCalculationType			m_backStubEndDateCalculationType;
		LTQC::Tenor						m_payDelay;
        LTQC::StubType		            m_stubType;
	    LT::date						m_spotFxDate;
    };  //  FloatingLegArguments

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_FLOATINGLEGARGUMENTS_H_INCLUDED