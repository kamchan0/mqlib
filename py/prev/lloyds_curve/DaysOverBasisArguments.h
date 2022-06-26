/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

/// \file

#ifndef __LIBRARY_PRICERS_FLEXYCF_DAYSOVERBASISARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DAYSOVERBASISARGUMENTS_H_INCLUDED

#include "LTQuantInitial.h"
#include "ModuleDate/InternalInterface/DayCounter.h"


namespace FlexYCF
{
    /// Represents the arguments as the key for caching
    /// basis calculations.
    class DaysOverBasisArguments
    {
    public:
        DaysOverBasisArguments(const ModuleDate::DayCounterConstPtr dayCounter,
                               const LT::date startDate,
                               const LT::date endDate);
        DaysOverBasisArguments& operator=(const DaysOverBasisArguments& other);

        ModuleDate::DayCounterConstPtr getBasis() const { return m_dayCounter;  }
        LT::date getStartDate() const           { return m_startDate;   }
        LT::date getEndDate()   const           { return m_endDate;     }

        double computeValue() const;

        struct Compare
        {
            bool operator()(const DaysOverBasisArguments& lhs, const DaysOverBasisArguments& rhs)
            {
                return (lhs.m_dayCounter->getName() < rhs.m_dayCounter->getName() ||
                    lhs.m_dayCounter->getName() == rhs.m_dayCounter->getName() && ( lhs.m_startDate < rhs.m_startDate ||
                    lhs.m_startDate == rhs.m_startDate && lhs.m_endDate < rhs.m_endDate)
                    );
            }
        };  //  Compare


        bool operator==(const DaysOverBasisArguments& other) const
        {
            return (m_startDate == other.m_startDate
                && m_endDate == other.m_endDate
                && m_dayCounter->getName() == other.m_dayCounter->getName());
        }

    private:
        ModuleDate::DayCounterConstPtr m_dayCounter;
        LT::date               m_startDate;
		LT::date               m_endDate;
    };

  
    size_t hash_value(const LT::date& date_);

    size_t hash_value(const DaysOverBasisArguments& daysOverBasisArguments);
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_DAYSOVERBASISARGUMENTS_H_INCLUDED