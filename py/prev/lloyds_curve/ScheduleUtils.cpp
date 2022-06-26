/*****************************************************************************
	ScheduleUtils
    
	
	@Originator

    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "ScheduleUtils.h"
#include "BaseModel.h"
#include "FixedLegArguments.h"
#include "GlobalComponentCache.h"

//	IDeA
#include "SettledBond.h"
#include "BondTradeFactory.h"
#include "DictMarketData.h"

using namespace LTQC;
using namespace LTQuant;
using namespace IDeA;

namespace FlexYCF
{
	const ILIndexArg getInflationIndexArguments(const LT::date& buildDate, const LT::date& observeDate, const string& timeLag, const string& resetInterp)
	{
		using namespace ModuleDate;

		const string noCalendarName("NOH");
		const CalendarPtr noCalendar(CalendarFactory::create(noCalendarName.c_str()));
		const LT::date forwardDate(addDatePeriod(observeDate, timeLag, noCalendar)); 

		const LT::date forward1Date(forwardDate.year(), forwardDate.month(), 1);

		const double forward1Time = getYearsBetweenAllowNegative(buildDate, forward1Date);

		const double weight = (static_cast<double>(observeDate.day()) - 1.0) / static_cast<double>(observeDate.end_of_month().day());

		// default to flat
		double forward2Time = forward1Time;

		// if we're not doing flat interpolation then work out the other weights
		if(compareNoCase(resetInterp, "Linear")) 
		{
			if (weight > 0.0) 
			{
				const LT::date forward2Date(DateFunctional::addMonths(forward1Date.getAsLong(), 1, true));
				forward2Time = getYearsBetweenAllowNegative(buildDate, forward2Date);
			}
		} 
		else if (!compareNoCase(resetInterp, "Flat"))
		{
			LT_THROW_ERROR( "Invalid reset interp type " << resetInterp );
		}
		
		return ILIndexArg(forward1Time, forward2Time, weight);
	}

	IDeA::SettledBondPtr createSetBondForInstrument(const LT::TablePtr& instrumentTbl, const LTQuant::GenericData& curveParametersTable, const LT::date& buildDate, 
													const LTQuant::GenericDataPtr& extraInfoTable, const IDeA::BondType& bondType /*= IDeA::BondType::FixedRateBond*/ )
	{
		const LT::Str emptyStr("");

		// handle Event table for settled bond
		LT::TablePtr eventsParams;
		if (extraInfoTable)
		{
			LT::TablePtr eventSchedule;
			IDeA::permissive_extract<LT::TablePtr>(extraInfoTable->table, IDeA_KEY(YIELDCURVE,EVENTSCHEDULE),eventSchedule, LT::TablePtr());
			if (eventSchedule)
			{
				eventsParams.reset(new LT::Table(2,1));
				eventsParams->at(0,0) = IDeA_PARAM(YIELDCURVE,EVENTSCHEDULE);
				eventsParams->at(1,0) = eventSchedule;
			}
		}

		// StaticData for settled bond
		LT::TablePtr staticDataParams;
		if (extraInfoTable)
			IDeA::permissive_extract<LT::TablePtr>(extraInfoTable->table, IDeA_KEY(YIELDCURVE,STATICDATA),staticDataParams, LT::TablePtr());

		// ISIN: required, but could be empty or arbitrary string, in which case market convention will be created from static
		const LT::Str Isin = IDeA::extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ISIN));

		// initialize bondDef with input bondType
		LT::TablePtr bondDefinitionParams(new LT::Table(2,1));
		size_t col = 0;
		bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,TYPE);
		IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,TYPE), 0, bondType.asString());

		// possible input fields to the bondDef
		LT::Str currency, calendar, category, issuer, basis, settlementDays, exDays, frequency, startDate, maturityDate, firstCouponDate, coupon, accrualDCM;

		// ****** create the bond definition table
		if (IDeA::permissive_extract<LT::Str>(curveParametersTable.table, IDeA_KEY(YC_CURVEPARAMETERS,CURRENCY), currency, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,CURRENCY);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,CURRENCY), 0, currency);
		}
		if (IDeA::permissive_extract<LT::Str>(curveParametersTable.table, IDeA_KEY(YC_CURVEPARAMETERS,HOLIDAYCALENDAR), calendar, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,CALENDAR);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,CALENDAR), 0, calendar);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,CATEGORY), category, emptyStr))
		{					
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,CATEGORY);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,CATEGORY), 0, category);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,ISSUER), issuer, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,ISSUER);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,ISSUER), 0, issuer);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,SETTLEMENTDAYS), settlementDays, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,SETTLEMENTDAYS);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,SETTLEMENTDAYS), 0, settlementDays);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,XDIVDAYS), exDays, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,XDIVDAYS);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,XDIVDAYS), 0, exDays);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,FREQUENCY), frequency, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,FREQUENCY);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,FREQUENCY), 0, frequency);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,STARTDATE), startDate, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,STARTDATE);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,STARTDATE), 0, startDate);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,ENDDATE), maturityDate, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,MATURITYDATE);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,MATURITYDATE), 0, maturityDate);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,FIRSTCOUPONDATE), firstCouponDate, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,FIRSTCOUPONDATE);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,FIRSTCOUPONDATE), 0, firstCouponDate);
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,COUPON), coupon, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			if (bondType == BondType::InflationBond)
			{
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,GEARING);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,GEARING), 0, coupon);
			}
			else
			{
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,COUPON);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,COUPON), 0, coupon);
			}
		}
		if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(YCI_INSTRUMENTPARAMETERS,ACCRUALDCM), accrualDCM, emptyStr))
		{
			++col;
			bondDefinitionParams->colAppend();
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,ACCRUALDCM);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,ACCRUALDCM), 0, accrualDCM);
		}

		return IDeA::BondTradeFactory::createSettledBond(Isin, bondDefinitionParams, staticDataParams, eventsParams, buildDate, Date());
	}

	std::vector<IDeA::SettledBondPtr> createSettledBondVec(const LTQuant::GenericDataPtr& instrumentTable, const LTQuant::GenericDataPtr& masterTable, const IDeA::BondType& bondType)
	{
		std::vector<IDeA::SettledBondPtr> setBondVec;

		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));

		const LT::TablePtr& instrumentTbl = instrumentTable->table;

		LT::TablePtr eventSchedule, eventsParams;
		IDeA::permissive_extract<LT::TablePtr>(masterTable->table, IDeA_KEY(YIELDCURVE,EVENTSCHEDULE),eventSchedule, LT::TablePtr());
		if (eventSchedule)
		{
			eventsParams.reset(new LT::Table(2,1));
			eventsParams->at(0,0) = IDeA_PARAM(YIELDCURVE,EVENTSCHEDULE);
			eventsParams->at(1,0) = eventSchedule;
		}
		LT::TablePtr staticDataParams;
		IDeA::permissive_extract<LT::TablePtr>(masterTable->table, IDeA_KEY(YIELDCURVE,STATICDATA),staticDataParams, LT::TablePtr());

		const LT::Str emptyStr("");

		for(size_t i = 0; i < IDeA::numberOfRecords(*instrumentTable); ++i)
		{
			// ISIN could be empty or arbitrary string, in which case market convention will be created from static
			const LT::Str Isin = IDeA::extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT, ISIN), i);

			// initialize bondDef with input bondType
			LT::TablePtr bondDefinitionParams(new LT::Table(2,1));
			size_t col = 0;
			bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,TYPE);
			IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,TYPE), 0, bondType.asString());

			// possible input fields to the bondDef
			LT::Str currency, calendar, category, issuer, basis, settlementDays, exDays, frequency, startDate, maturityDate, firstCouponDate, coupon, accrualDCM;

			// ****** create the bond definition table
			if (IDeA::permissive_extract<LT::Str>(parametersTable->table, IDeA_KEY(YC_CURVEPARAMETERS,CURRENCY), currency, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,CURRENCY);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,CURRENCY), 0, currency);
			}
			if (IDeA::permissive_extract<LT::Str>(parametersTable->table, IDeA_KEY(YC_CURVEPARAMETERS,HOLIDAYCALENDAR), calendar, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,CALENDAR);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,CALENDAR), 0, calendar);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,CATEGORY), i, category, emptyStr))
			{					
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,CATEGORY);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,CATEGORY), 0, category);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,ISSUER), i, issuer, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,ISSUER);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,ISSUER), 0, issuer);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,SETTLEMENTDAYS), i, settlementDays, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,SETTLEMENTDAYS);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,SETTLEMENTDAYS), 0, settlementDays);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,XDIVDAYS), i, exDays, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,XDIVDAYS);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,XDIVDAYS), 0, exDays);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,FREQUENCY), i, frequency, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,FREQUENCY);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,FREQUENCY), 0, frequency);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,STARTDATE), i, startDate, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,STARTDATE);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,STARTDATE), 0, startDate);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,MATURITYDATE), i, maturityDate, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,MATURITYDATE);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,MATURITYDATE), 0, maturityDate);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,FIRSTCOUPONDATE), i, firstCouponDate, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,FIRSTCOUPONDATE);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,FIRSTCOUPONDATE), 0, firstCouponDate);
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,COUPON), i, coupon, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				if (bondType == BondType::InflationBond)
				{
					bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,GEARING);
					IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,GEARING), 0, coupon);
				}
				else
				{
					bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,COUPON);
					IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,COUPON), 0, coupon);
				}
			}
			if (IDeA::permissive_extract<LT::Str>(instrumentTbl, IDeA_KEY(BONDINSTRUMENT,ACCRUALDCM), i, accrualDCM, emptyStr))
			{
				++col;
				bondDefinitionParams->colAppend();
				bondDefinitionParams->at(0,col) = IDeA_PARAM(BONDDEFINITION,ACCRUALDCM);
				IDeA::inject(*bondDefinitionParams, IDeA_KEY(BONDDEFINITION,ACCRUALDCM), 0, accrualDCM);
			}
			// ***** finish bond definition table

			// create settled bond:
			setBondVec.push_back(IDeA::BondTradeFactory::createSettledBond(Isin, bondDefinitionParams, staticDataParams, eventsParams, Date(valueDate), Date()));
		}

		return setBondVec;
	}

	void setFixedLegCvgPayDates( FixedLegPtr& fixedLeg, const IDeA::SettledBondConstPtr& setBond )
	{
		using namespace std;
		using namespace std::placeholders;

		const std::vector<BondFlowInfoPtr>& bondFlowInfo = setBond->getBondFlowInfo();
		const int num = setBond->m_xbond->getNumberOfCouponsPerYear();

		std::vector<Date>& payDates = fixedLeg->getPaymentDates();
		std::vector<double>& cvg = fixedLeg->getCvg();
		ModuleDate::Schedule::ScheduleEvents& scheduleDates = fixedLeg->getSchedule();

		// only populate the coupon flows, the redemption flow need to be handled separately
		std::for_each(	bondFlowInfo.begin(), bondFlowInfo.end() - 1, 
						[&payDates, &cvg, &scheduleDates, &num](const BondFlowInfoPtr& bf)
						{	cvg.push_back(bf->m_timeFraction / num); payDates.push_back(bf->m_payDate);
							scheduleDates.push_back(LT::DatePeriod(bf->m_adjustedStartDate.getAsLTdate(), bf->m_adjustedEndDate.getAsLTdate()));
						}
					);
	}

	FixedLegPtr createFixedLegArgFromSetBond( const IDeA::SettledBondPtr& setBond )
	{
		const LT::date& startDate = setBond->m_contractDate.getAsLTdate();
		const LT::date& endDate = setBond->m_xbond->getLastPaymentDate().getAsLTdate();

		return 
			FixedLeg::create
			(
				FixedLeg::Arguments(setBond->m_contractDate.getAsLTdate(), 
				startDate, 
				startDate, 
				endDate, 
				setBond->m_xbond->m_bond->m_bondDefinition->m_frequency.string(), 
				// The following basis is arbitrarily set, since ActActISMA is not implemented in QDDate and therefore could not retrieved from setBond.
				// However it doesn't matter since m_basis is not used for bond instruments
				"Act/365",
				setBond->m_xbond->m_bond->m_bondDefinition->m_accrualCalendar.string(),
				setBond->getCurrency(),
				"",
				RollConvMethod(setBond->m_xbond->m_bond->m_bondDefinition->m_accrualRollConvention.asString()),
				setBond->m_xbond->m_bond->m_bondDefinition->m_accrualDurationModifyConvention,
				"0B",
				setBond->m_xbond->m_bond->m_bondDefinition->m_calendar.string(),
				setBond->m_xbond->m_bond->m_bondDefinition->m_rollConvention.asString(),
				setBond->m_xbond->m_bond->m_bondDefinition->m_stubType)
			);
	}

	FixedLegPtr createFixedLegArgFromSetBond( const IDeA::SettledBondPtr& setBond, GlobalComponentCache& globalComponentCache )
	{
		const LT::date& startDate = setBond->m_contractDate.getAsLTdate();
		const LT::date& endDate = setBond->m_xbond->getLastPaymentDate().getAsLTdate();
		return 
			globalComponentCache.get
			(
				FixedLeg::Arguments(setBond->m_contractDate.getAsLTdate(), 
				startDate, 
				startDate, 
				endDate, 
				setBond->m_xbond->m_bond->m_bondDefinition->m_frequency.string(),
				// The following basis is arbitrarily set, since ActActISMA is not implemented in QDDate and therefore could not retrieved from setBond.
				// However it doesn't matter since m_basis is not used for bond instruments
				"Act/365",	
				setBond->m_xbond->m_bond->m_bondDefinition->m_accrualCalendar.string(),
				setBond->getCurrency(),
				"",
				RollConvMethod(setBond->m_xbond->m_bond->m_bondDefinition->m_accrualRollConvention.asString()),
				setBond->m_xbond->m_bond->m_bondDefinition->m_accrualDurationModifyConvention,
				"0B",
				setBond->m_xbond->m_bond->m_bondDefinition->m_calendar.string(),
				setBond->m_xbond->m_bond->m_bondDefinition->m_rollConvention.asString(),
				setBond->m_xbond->m_bond->m_bondDefinition->m_stubType)
			);
	}
} // FlexYCF