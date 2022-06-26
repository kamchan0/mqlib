#include "stdafx.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"

//	FlexYCF
#include "GlobalComponentCache.h"
#include "InstrumentComponentCache.h" 
#include "Data\GenericData.h"
#include "CalibrationInstrument.h"
#include "ModuleDate/InternalInterface/CalendarFactory.h"

using namespace std;
using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
     GlobalComponentCache::GlobalComponentCache(const bool cacheDiscountFactorCalculations,
                                                const bool cacheTenorDiscountFactorCalculations,
                                                const bool cacheForwardRateCalculations,
                                                const bool cacheDiscountedForwardRateCalculations,
                                                const bool cacheFixedLegCalculations,
                                                const bool cacheFloatingLegCalculations,
                                                const bool cacheFixedCashFlowCalculations,
												const bool cacheInflationIndexCalculations) :

        m_discountFactorCache       (cacheDiscountFactorCalculations),
        m_tenorDiscountFactorCache  (cacheTenorDiscountFactorCalculations),
        m_forwardRateCache          (cacheForwardRateCalculations),
        m_discountedForwardRateCache(cacheDiscountedForwardRateCalculations),
        m_fixedLegCache             (cacheFixedLegCalculations),
        m_floatingLegCache          (cacheFloatingLegCalculations),
        m_fixedCashFlowCache        (cacheFixedCashFlowCalculations),
		m_inflationIndexCache		(cacheInflationIndexCalculations),

        m_daysOverBasisCache        (new DaysOverBasisCache)
    {
    }


    GlobalComponentCache::GlobalComponentCache(const LT::date buildDate,
                                               const ModuleDate::CalendarPtr calendar,
                                               const bool cacheDiscountFactorCalculations,
                                               const bool cacheTenorDiscountFactorCalculations,
                                               const bool cacheForwardRateCalculations,
                                               const bool cacheDiscountedForwardRateCalculations,
                                               const bool cacheFixedLegCalculations,
                                               const bool cacheFloatingLegCalculations,
                                               const bool cacheFixedCashFlowCalculations,
											   const bool cacheInflationIndexCalculations) :

        m_discountFactorCache       (cacheDiscountFactorCalculations),
        m_tenorDiscountFactorCache  (cacheTenorDiscountFactorCalculations),
        m_forwardRateCache          (cacheForwardRateCalculations),
        m_discountedForwardRateCache(cacheDiscountedForwardRateCalculations),
        m_fixedLegCache             (cacheFixedLegCalculations),
        m_floatingLegCache          (cacheFloatingLegCalculations),
        m_fixedCashFlowCache        (cacheFixedCashFlowCalculations),
		m_inflationIndexCache		(cacheInflationIndexCalculations),

        m_daysOverBasisCache        (new DaysOverBasisCache)
    {
       // m_instrumentComponentCaches.push_back(InstrumentComponentCachePtr(new InstrumentComponentCache<DiscountFactor>()));
       // registerComponent<DiscountFactor>();

        fillScheduleEvents(buildDate, calendar);
    }

    GlobalComponentCache GlobalComponentCache::createCache(const LTQuant::GenericDataPtr masterTable,
                                                           const LT::date buildDate)
    {
		const GenericDataPtr modelParametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS)));
        const GenericDataPtr curveParametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));

        const bool cacheTableExists(modelParametersTable->doesTagExist(IDeA_PARAM(FLEXYC_MODELPARAMETERS,CACHEPARAMETERS)) != NULL);
        LTQuant::GenericDataPtr cacheParameters;
        if(cacheTableExists)
        {
			cacheParameters = IDeA::extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, CACHEPARAMETERS));
        }
        
        bool cacheDiscountFactorCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Discount Factor Cache") && cacheParameters->get<int>("Discount Factor Cache", 0) != 0)
            cacheDiscountFactorCalculations = true;
        
        bool cacheTenorDiscountFactorCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Tenor DF Cache") && cacheParameters->get<int>("Tenor DF Cache", 0) != 0)
            cacheTenorDiscountFactorCalculations = true;

        bool cacheForwardRateCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Fwd Rate Cache") && cacheParameters->get<int>("Fwd Rate Cache", 0) != 0)
            cacheForwardRateCalculations = true;

        bool cacheDiscountedForwardRateCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Discounted Fwd Rate Cache") && cacheParameters->get<int>("Discounted Fwd Rate Cache", 0) != 0)
            cacheDiscountedForwardRateCalculations = true;

        bool cacheFixedLegCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Fxd Leg Cache") && cacheParameters->get<int>("Fxd Leg Cache", 0) != 0)
            cacheFixedLegCalculations = true;

        bool cacheFloatingLegCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Flt Leg Cache") && cacheParameters->get<int>("Flt Leg Cache", 0) != 0)
            cacheFloatingLegCalculations = true;

        bool cacheFixedCashFlowCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Fxd CF") && cacheParameters->get<int>("Fxd CF", 0) != 0)
            cacheFixedCashFlowCalculations = true;

		bool cacheInflationIndexCalculations(false);
		if(cacheTableExists && cacheParameters->doesTagExist("Inflation Index Cache") && cacheParameters->get<int>("Inflation Index", 0) != 0)
			cacheInflationIndexCalculations = true;

        ModuleDate::CalendarPtr calendar;
        if(curveParametersTable->doesTagExist("Currency") && curveParametersTable->doesTagExist("Forecast Index"))
        {
            calendar = ModuleDate::CalendarFactory::create(createIRCurveMktConventions(curveParametersTable)->m_depo.m_accrualValueCalendar.string());
        }
        else
        {
            calendar = ModuleDate::CalendarFactory::create("NOH");
        }

        return GlobalComponentCache(buildDate,
                                    calendar,
                                    cacheDiscountFactorCalculations,
                                    cacheTenorDiscountFactorCalculations,
                                    cacheForwardRateCalculations,
                                    cacheDiscountedForwardRateCalculations,
                                    cacheFixedLegCalculations,
                                    cacheFloatingLegCalculations,
                                    cacheFixedCashFlowCalculations,
									cacheInflationIndexCalculations);
    }

	GlobalComponentCache GlobalComponentCache::createCache(const LTQuant::GenericDataPtr& masterTable)
	{
		const LTQuant::GenericDataPtr curveDetailsTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date buildDate(IDeA::extract<LT::date>(*curveDetailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		
		const GenericDataPtr modelParametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS)));
        const GenericDataPtr curveParametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));

        const bool cacheTableExists(modelParametersTable->doesTagExist(IDeA_PARAM(FLEXYC_MODELPARAMETERS,CACHEPARAMETERS)) != NULL);
        LTQuant::GenericDataPtr cacheParameters;
        if(cacheTableExists)
        {
			cacheParameters = IDeA::extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, CACHEPARAMETERS));
        }
        
        bool cacheDiscountFactorCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Discount Factor Cache") && cacheParameters->get<int>("Discount Factor Cache", 0) != 0)
            cacheDiscountFactorCalculations = true;
        
        bool cacheTenorDiscountFactorCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Tenor DF Cache") && cacheParameters->get<int>("Tenor DF Cache", 0) != 0)
            cacheTenorDiscountFactorCalculations = true;

        bool cacheForwardRateCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Fwd Rate Cache") && cacheParameters->get<int>("Fwd Rate Cache", 0) != 0)
            cacheForwardRateCalculations = true;

        bool cacheDiscountedForwardRateCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Discounted Fwd Rate Cache") && cacheParameters->get<int>("Discounted Fwd Rate Cache", 0) != 0)
            cacheDiscountedForwardRateCalculations = true;

        bool cacheFixedLegCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Fxd Leg Cache") && cacheParameters->get<int>("Fxd Leg Cache", 0) != 0)
            cacheFixedLegCalculations = true;

        bool cacheFloatingLegCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Flt Leg Cache") && cacheParameters->get<int>("Flt Leg Cache", 0) != 0)
            cacheFloatingLegCalculations = true;

        bool cacheFixedCashFlowCalculations(false);
        if(cacheTableExists && cacheParameters->doesTagExist("Fxd CF") && cacheParameters->get<int>("Fxd CF", 0) != 0)
            cacheFixedCashFlowCalculations = true;

		bool cacheInflationIndexCalculations(false);
		if(cacheTableExists && cacheParameters->doesTagExist("Inflation Index Cache") && cacheParameters->get<int>("Inflation Index Cache", 0) != 0)
			cacheInflationIndexCalculations = true;

        ModuleDate::CalendarPtr calendar;
        if(curveParametersTable->doesTagExist("Currency") && curveParametersTable->doesTagExist("Forecast Index"))
        {
            calendar = ModuleDate::CalendarFactory::create(createIRCurveMktConventions(curveParametersTable)->m_depo.m_accrualValueCalendar.string());
        }
        else
        {
            calendar = ModuleDate::CalendarFactory::create("NOH");
        }

        return GlobalComponentCache(buildDate,
                                    calendar,
                                    cacheDiscountFactorCalculations,
                                    cacheTenorDiscountFactorCalculations,
                                    cacheForwardRateCalculations,
                                    cacheDiscountedForwardRateCalculations,
                                    cacheFixedLegCalculations,
                                    cacheFloatingLegCalculations,
                                    cacheFixedCashFlowCalculations,
									cacheInflationIndexCalculations);
	}
        
    const ModuleDate::Schedule::ScheduleEvents& GlobalComponentCache::getScheduleEvents(const string& tenorDescription) const
    {
        TenorScheduleEventsContainer::const_iterator lower(lower_bound(m_scheduleEventsList.begin(),
                                                                       m_scheduleEventsList.end(),
                                                                       tenorDescription,
                                                                       TenorCompare()) );

        if(lower == m_scheduleEventsList.end() || TenorCompare()(tenorDescription, *lower))
        {
            LT_THROW_ERROR( "The tenor description specified is not recognized: it must be 1M, 3M, 6M or 1Y/12M");
        }
        return lower->second;
    }

    void GlobalComponentCache::fillScheduleEvents(const LT::date buildDate,
                                                  const ModuleDate::CalendarPtr calendar)
    {
        vector<string> tenors;
        tenors.push_back("1M");
        tenors.push_back("3M");
        tenors.push_back("6M");
        tenors.push_back("1Y");

        sort(tenors.begin(), tenors.end());

        const LT::date endDate(addDatePeriod(buildDate, "60Y", calendar));

        ModuleDate::ScheduleGeneratorPtr       scheduleGenerator;
        ModuleDate::Schedule::ScheduleEvents   scheduleEvents;

        for(vector<string>::const_iterator iter(tenors.begin()); iter != tenors.end(); ++iter)
        {
            // cout << (*iter) << endl;

            m_scheduleEventsList.push_back(TenorScheduleEventsPair(*iter, ModuleDate::Schedule::ScheduleEvents()));
            scheduleGenerator = ModuleDate::ScheduleGeneratorFactory::create(*iter, calendar);
            
            TenorScheduleEventsContainer::iterator lower(lower_bound(m_scheduleEventsList.begin(),
                                                                     m_scheduleEventsList.end(),
                                                                     *iter,
                                                                     TenorCompare()));

            if(lower == m_scheduleEventsList.end() || TenorCompare()(*iter, *lower))
            {
                LT_THROW_ERROR( "Error" );
            }

            scheduleGenerator->fillEvents(lower->second, buildDate, endDate);

        }

        // Add 12M as well 
        TenorScheduleEventsContainer::iterator lower(lower_bound(m_scheduleEventsList.begin(),
                                                                 m_scheduleEventsList.end(),
                                                                 "12M",
                                                                 TenorCompare()));
        
        m_scheduleEventsList.insert(lower, TenorScheduleEventsPair("12M", getScheduleEvents("1Y")));
    }

}   //  FlexYCF