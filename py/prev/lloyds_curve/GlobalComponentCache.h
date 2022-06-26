/*****************************************************************************

    GlobalComponentCache

	Encapsulates all caches ofinstrument component value and gradient
	calculations.

    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_GLOBALCOMPONENTCACHE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_GLOBALCOMPONENTCACHE_H_INCLUDED
#pragma once
#include "AllComponentsAndCaches.h"
#include "DaysOverBasisCache.h"
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
    FWD_DECLARE_SMART_PTRS( Calendar )
}

namespace FlexYCF
{
    /// GlobalComponentCache contains all the templatized
    /// instrument component caches.
    class GlobalComponentCache
    {
    public:
         explicit GlobalComponentCache(const bool cacheDiscountFactorCalculations        = true,
                                       const bool cacheTenorDiscountFactorCalculations   = true,
                                       const bool cacheForwardRateCalculations           = true,
                                       const bool cacheDiscountedForwardRateCalculations = true,
                                       const bool cacheFixedLegCalculations              = true,
                                       const bool cacheFloatingLegCalculations           = true,
                                       const bool cacheFixedCashFlowCalculations         = true,
									   const bool cacheInflationIndexCalculations		 = true);

        explicit GlobalComponentCache(const LT::date buildDate,
                                      const ModuleDate::CalendarPtr calendar,
                                      const bool cacheDiscountFactorCalculations        = true,
                                      const bool cacheTenorDiscountFactorCalculations   = true,
                                      const bool cacheForwardRateCalculations           = true,
                                      const bool cacheDiscountedForwardRateCalculations = true,
                                      const bool cacheFixedLegCalculations              = true,
                                      const bool cacheFloatingLegCalculations           = true,
                                      const bool cacheFixedCashFlowCalculations         = true,
									  const bool cacheInflationIndexCalculations		= true);

        static GlobalComponentCache createCache(const LTQuant::GenericDataPtr data, const LT::date buildDate);
		static GlobalComponentCache createCache(const LTQuant::GenericDataPtr& masterTable);
	
       
		
        //  Discount Factor 
        DiscountFactorPtr get(const DiscountFactor::Arguments& arguments)
        {
            return m_discountFactorCache.get(arguments);
        }
    
        // Tenor Discount Factor
        TenorDiscountFactorPtr get(const TenorDiscountFactor::Arguments& arguments)
        {
            return m_tenorDiscountFactorCache.get(arguments);
        }
     
        //  Forward Rate
        ForwardRatePtr get(const ForwardRate::Arguments& arguments)
        {
            return m_forwardRateCache.get(arguments);
        }
         
        //  Discounted Forward Rate
        DiscountedForwardRatePtr get(const DiscountedForwardRate::Arguments& arguments)
        {
            return m_discountedForwardRateCache.get(arguments);
        }
        
        //  Fixed Leg
        FixedLegPtr get(const FixedLeg::Arguments& arguments)
        {
            return m_fixedLegCache.get(arguments);
        }

        //  Floating Leg
        FloatingLegPtr get(const FloatingLeg::Arguments& arguments)
        {
            return m_floatingLegCache.get(arguments);
        }

        //  FixedCashFlow
        FixedCashFlowPtr get(const FixedCashFlow::Arguments& arguments)
        {
            return m_fixedCashFlowCache.get(arguments);
            //return FixedCashFlow::create(arguments);
        }

		//  Inflation Index 
		InflationIndexPtr get(const InflationIndex::Arguments& arguments)
		{
			return m_inflationIndexCache.get(arguments);
		}

        /*
        // Days over Basis
        double getDaysOverBasis(const DayCounterConstPtr dayCounter, const date startDate, const date endDate)
        {
            return m_daysOverBasisCache->getDaysOverBasis(dayCounter, startDate, endDate);
        }
        */
       
        // This is to be used by fixed and floating leg component arguments (FixedLegArguments and FloatingLegArguments)
        const ModuleDate::Schedule::ScheduleEvents& getScheduleEvents(const std::string& tenorDescription) const;

    private:
        typedef std::pair<std::string, ModuleDate::Schedule::ScheduleEvents> TenorScheduleEventsPair;
        typedef std::vector<TenorScheduleEventsPair> TenorScheduleEventsContainer;
    
    public:
        struct TenorCompare
        {
            bool operator()(const TenorScheduleEventsPair& lhs, const TenorScheduleEventsPair& rhs) //const
            {
                return lhs.first < rhs.first;
            }
            
            bool operator()(const TenorScheduleEventsPair& lhs, const std::string& rhs) //const
            {
                return lhs.first < rhs;
            }

            bool operator()(const std::string& lhs, const TenorScheduleEventsPair& rhs) //const
            {
                return lhs < rhs.first;
            }

        };
    private:
        void fillScheduleEvents(const LT::date buildDate,
		const ModuleDate::CalendarPtr calendar);
        DiscountFactorCache         m_discountFactorCache;
        TenorDiscountFactorCache    m_tenorDiscountFactorCache;
        ForwardRateCache            m_forwardRateCache;
        DiscountedForwardRateCache  m_discountedForwardRateCache;
        FixedLegCache               m_fixedLegCache;
        FloatingLegCache            m_floatingLegCache;
        FixedCashFlowCache          m_fixedCashFlowCache;
		InflationIndexCache			m_inflationIndexCache;

        DaysOverBasisCachePtr       m_daysOverBasisCache;

        TenorScheduleEventsContainer m_scheduleEventsList; // filled with the appropriate ctor, but not used by instr. components yet
    };  // GlobalComponentCache

}
#endif //__LIBRARY_PRICERS_FLEXYCF_GLOBALCOMPONENTCACHE_H_INCLUDED