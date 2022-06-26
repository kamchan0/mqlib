/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DAYSOVERBASISCACHE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DAYSOVERBASISCACHE_H_INCLUDED
#pragma once

#include "Cache.h"
#include "DaysOverBasisArguments.h"
#include "ModuleDate/InternalInterface/DayCounter.h"


namespace FlexYCF
{
    /// A class to cache computations of day counter's getDaysOverBasis
    /// function.
    class DaysOverBasisCache
    {
    public:
        DaysOverBasisCache()
        {
			m_cache.setFunction([] (const DaysOverBasisArguments& daysOverBasisArguments)
				{return daysOverBasisArguments.computeValue();});
        }

        double getDaysOverBasis(const ModuleDate::DayCounterConstPtr dayCounter, const LT::date startDate, const LT::date endDate)
        {
            return m_cache.get(DaysOverBasisArguments(dayCounter, startDate, endDate));    
        }

    private:
        Cache<DaysOverBasisArguments, double, DaysOverBasisArguments::Compare> m_cache;    
    };  // DayCounterCache

    DECLARE_SMART_PTRS( DaysOverBasisCache )

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_DAYSOVERBASISCACHE_H_INCLUDED