#include "stdafx.h"
#include "DaysOverBasisArguments.h"
#include "ModuleDate/InternalInterface/DayCounter.h"

namespace FlexYCF
{
    DaysOverBasisArguments::DaysOverBasisArguments(const ModuleDate::DayCounterConstPtr dayCounter,
                                                   const LT::date startDate,
                                                   const LT::date endDate):
        m_dayCounter(dayCounter),
        m_startDate(startDate),
        m_endDate(endDate)
    {
    }

    DaysOverBasisArguments& DaysOverBasisArguments::operator=(const DaysOverBasisArguments& other)
    {
        if(&other != this)
        {
            m_dayCounter    = other.m_dayCounter;
            m_startDate     = other.m_startDate;
            m_endDate       = other.m_endDate;
        }
        return *this;
    }
    
    double DaysOverBasisArguments::computeValue() const
    {
        return m_dayCounter->getDaysOverBasis(m_startDate, m_endDate);
    }

}   //  FlexYCF