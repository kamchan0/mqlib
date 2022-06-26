/*****************************************************************************
    
	CalibrationInstrument

	This file contains the implementation of the CalibrationInstrument.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	Standard
#include <iostream>
#include <sstream>
#include <string>

//	FlexYCF
#include "CalibrationInstrument.h"
#include "CurveType.h"
#include "NullDeleter.h"

//	LTQuantLib
#include "Static/FuturesPropertiesFactory.h"
#include "Data/GenericData.h"

#include "DateUtils.h"

//	LTQC
#include "QCUtils.h"
// #include "UtilsEnums.h"

//	IDeA
#include "TradeConventionCalcH.h"
#include "DataExtraction.h"
#include "DictYieldCurve.h"
#include "DictionaryManager.h"
#include "Exception.h"
#include "IRDepositRate.h"

// ModuleStaticData
#include "ModuleStaticData/InternalInterface/IRIndexProperties.h"
#include "ModuleStaticData/InternalInterface/CurrencyPairProperties.h"

// ModuleDate
#include "ModuleDate/InternalInterface/FXFunctionalInterface.h"

using namespace std;
using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
	/*
    CalibrationInstrument::CalibrationInstrument(const string& description,
                                                 const date fixingDate,
                                                 const date startDate,
                                                 const date endDate,
                                                 const double rate) :
        // m_description(description),
		// m_description2(description),
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
		// m_rate(rate),
		m_wasPlaced(false)
    {
    }
	*/
	CalibrationInstrument::CalibrationInstrument(const double rate,
											     const LT::Str& name,
											     const LT::Str& description,
											     const LT::date fixingDate,
											     const LT::date startDate,
												 const LT::date endDate,
												 const LT::Str& identity):
		CachedDerivInstrument(rate, name, description),
	    m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
		m_wasPlaced(false),
        m_flowsRemoved(false)
	{
		if (identity == "")
			m_identity = getDescription();
		else
			m_identity = identity;
	}
	/*
	void CalibrationInstrument::addRateToProblem(LTQuant::Problem& problem)
	{
		problem.addVariable(VariablePtr(new SolverVariable(m_rate)));
	}
	*/

    ostream& CalibrationInstrument::print(ostream& out) const
    {
        return out;
    }
    
    CalibrationInstrument::CalibrationInstrument()
    {
    }

	
	void CalibrationInstrument::setStartDate(const LT::date startDate)
	{
		m_startDate = startDate;
	}

	void CalibrationInstrument::setEndDate(const LT::date endDate)
	{
		m_endDate = endDate;
	}


    //there is an implementation in this class that is responsible for capturing
    //the cached values
    void CalibrationInstrument::finishCalibration(const BaseModelPtr model)
    {
        if(!m_flowsRemoved)
        {
            //don't care about valuation exceptions. The value will
            //stay uninitilised and we will throw on first use of it
            try
            {
                 setBPV(computeBPV(model));
            }
            catch(...)
            {}
            try
            {
                 setRateDerivative(calculateRateDerivative(model));
            }
            catch(...)
            {}
            m_flowsRemoved=true;
        }
    }
	 
	void CalibrationInstrument::setValues(const BaseModelPtr model)
    {
          setBPV(computeBPV(model));
          setRateDerivative(calculateRateDerivative(model));
    }
    /// after finishCalibration the instrument is an empty shell that cannot value itself
    /// given a full representation of the instrument as src refresh yourself with
    /// enough information so it is possible to value this instr again
    void CalibrationInstrument::reloadInternalState(const CalibrationInstrumentPtr&)
    {
        if(!m_flowsRemoved)
        {
           LTQC_THROW(IDeA::Exception, "Cannot reload instrument state when this calibration instrument has flows");
        }
        m_flowsRemoved=false;
    }
	LTQuant::GenericDataPtr CalibrationInstrument::getCashFlows()
	{
		LTQuant::GenericDataPtr cashFlowsTable(new LTQuant::GenericData("Cash Flows", 0));

		fillCashFlowsTable(*cashFlowsTable);

		return cashFlowsTable;
	}

	void CalibrationInstrument::fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const
	{
		cashFlowsTable.set<LT::date>("Start Date", 0, m_startDate);
		cashFlowsTable.set<LT::date>("End Date", 0, m_endDate);
	}

	LTQuant::GenericDataPtr CalibrationInstrument::computeCashFlowPVs(const BaseModel& model)
	{
		LTQuant::GenericDataPtr cashFlowPVsTable(new LTQuant::GenericData("Cash Flow PVs", 0));

		fillCashFlowPVsTable(model, *cashFlowPVsTable);

		return cashFlowPVsTable;
	}

	void CalibrationInstrument::fillCashFlowPVsTable(const BaseModel& model,
													 LTQuant::GenericData& cashFlowPVsTable) const
	{
		cashFlowPVsTable.set<LT::date>("Start Date", 0, m_startDate);
		cashFlowPVsTable.set<LT::date>("End Date", 0, m_endDate);
		cashFlowPVsTable.set<double>("Rate", 0, getRate());
		// don't have the value date to calculate the discount factor
		//	cashFlowPVsTable.set<double>("PV", 0, m_rate * getYearsBetween(m_startDate, m_endDate) * model.getDiscountFactor(getYeasrBetween(m_valueDate)
	}

	

	double CalibrationInstrument::doGetDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData,
															 const IDeA::DictionaryKey& instrumentKey,
															 const IDeA::DictionaryKey& descriptionKey,
															 const IDeA::DictionaryKey& rateKey) const
	{
		LTQuant::GenericDataPtr instrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(instrumentListData, instrumentKey, instrumentData);

		if(!instrumentData)
		{
			LTQC_THROW( LTQC::DataQCException, "Cannot find the instrument table \"" << instrumentKey.getName().string() << "\"" );
		}

		double newRate;
		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentData));
		
		for(size_t i(0); i < nbInstruments; ++i)
		{
			if(compareAndTryGetNewRate(*instrumentData, i, descriptionKey, rateKey, newRate))
			{
				return newRate - getRate();
			}
		}
		
		LTQC_THROW( LTQC::DataQCException, "Cannot find an instrument with description \"" << getDescription().string() << "\" in table \"" << instrumentKey.getName().string() << "\"" );
	}

	bool CalibrationInstrument::compareAndTryGetNewRate(const LTQuant::GenericData& instrumentData,
														const size_t index,
														const IDeA::DictionaryKey& descriptionKey,
														const IDeA::DictionaryKey& rateKey,
														double& newRate) const
	{
		if(IDeA::extract<std::string>(instrumentData, descriptionKey, index) == getDescription().string())
		{
			return IDeA::permissive_extract<double>(instrumentData, rateKey, index, newRate);					
		}
		return false;
	}

	IDeA::IRCurveMktConventionPtr createIRCurveMktConventions(const LTQuant::GenericData& curveParametersTable, const ModuleStaticData::IRIndexPropertiesPtr& indexProperties)
	{
		const std::string currency(IDeA::extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY)));
		const std::string index(IDeA::extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX)));
		
		LTQC::Currency ccy(currency);

		//	Spot Days
        std::string const spotDaysStrDefault = LTQC::Tenor::tenorString(indexProperties->getSpotDays(), 'B');
		std::string spotDaysStr;
		IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, SPOTDAYS), spotDaysStr, spotDaysStrDefault);
		if (spotDaysStr.empty()) spotDaysStr = spotDaysStrDefault;
		LTQC::Tenor spotDays(spotDaysStr.c_str());

		//	Basis	
		std::string basisDefault(indexProperties->getBasis()->getName());
		std::string basis;
		IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, BASIS),basis, basisDefault);
		if (basis.empty()) basis = basisDefault;
		LTQC::DayCountMethod floatBasis(basis.c_str());

		//	Deposit Fixing Calendar	
		std::string fixingCalendarDefault(indexProperties->getFixingCalendarName());
		std::string fixingCalendar;
		bool fixingCalendarProvided = IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FIXINGCALENDAR), fixingCalendar, fixingCalendarDefault);
		if (fixingCalendar.empty()) fixingCalendar = fixingCalendarDefault;

		//	Deposit Reset Value date Holiday Calendar	
		std::string settleCalendarDefault(indexProperties->getCalendarName());
		std::string settleCalendar;
		IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, HOLIDAYCALENDAR), settleCalendar, settleCalendarDefault);
		if (settleCalendar.empty()) settleCalendar = settleCalendarDefault;

		// Calendars
		std::string fixingAccCalendar = settleCalendar;
		std::string floatAccCalendar = settleCalendar;
		std::string fixedAccCalendar = floatAccCalendar;

		//	Float Tenor		
        std::string const floatTenorStrDefault = LTQC::Tenor::tenorString(indexProperties->getSwapFloatPeriod(), 'M');
		std::string floatTenorStr;
		IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FLOATTENOR), floatTenorStr, floatTenorStrDefault);
		if (floatTenorStr.empty()) floatTenorStr = floatTenorStrDefault;
		LTQC::Tenor floatTenor(floatTenorStr.c_str());

		// Roll Convention
		LTQC::RollConvMethod rollConv(LTQC::RollConvMethod::ModifiedFollowing);
		LTQC::RollRuleMethod rollRuleConv(LTQC::RollRuleMethod::BusinessEOM);

        // Futures contract size - cannot guarantee we will get back futures definition
        LT::Ptr<LT::Table> futuresProperties( FuturesPropertiesFactory::create( currency/*, index */) ); // Futures index not same as currency index
// There was an assumption that the default size was adequate in this onctest. As we no longer return an unknown future definition
// we will default the cotract to that original default value. Note this code previously always returned the default 
        double contractSize(500000);
        double futuresContractSize;
        if ( futuresProperties.get() )
        {
            contractSize = futuresProperties->at( 1, "Unit Size" ).convertTo<double>();
        }
        const double defaultContractSize(contractSize);
        IDeA::permissive_extract< double >(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FUTURESCONTRACTSIZE), futuresContractSize, defaultContractSize );
        // Futures contract prefix for this currency
        std::string const contractDescriptionPrefix = indexProperties->getFuturesContractDescriptionPrefix(currency, index);

		LTQC::Tenor futureTenor(0, 0, 0, 3, 0);
		IDeA::DepositRateMktConvention depoConvention(spotDays, ccy, LT::Str(index), floatBasis, floatTenor, LT::Str(settleCalendar), rollConv,rollRuleConv, fixingAccCalendar, LTQC::RollConvMethod(LTQC::RollConvMethod::None));
		depoConvention.m_fixingCalendar = LT::Str(fixingCalendar);
		IDeA::FutureMktConvention futureConvention(spotDays, ccy, LT::Str(index), floatBasis, futureTenor, LT::Str(settleCalendar), rollConv, 
                                    LT::Str(contractDescriptionPrefix), futuresContractSize);
		if( fixingCalendarProvided )
			futureConvention.m_fixingCalendar = depoConvention.m_fixingCalendar;

        // FRA
        IDeA::DepositRateMktConvention fraDepoConvention = depoConvention;
        IDeA::FRAMktConvention fraConvention( fraDepoConvention );

		//	Fixed Tenor						
        std::string const fixedTenorStrDefault = LTQC::Tenor::tenorString(indexProperties->getSwapFixedPeriod(), 'M');
		std::string fixedTenorStr;
		IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FIXEDTENOR), fixedTenorStr, fixedTenorStrDefault);
		if (fixedTenorStr.empty()) fixedTenorStr = fixedTenorStrDefault;
		LTQC::Tenor fixedTenor(fixedTenorStr.c_str());

		//  Fixed Basis
		std::string fixedBasisStrDefault(indexProperties->getSwapFixedBasis()->getName());
		std::string fixedBasisStr;
		IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FIXEDBASIS), fixedBasisStr, fixedBasisStrDefault);
		if (fixedBasisStr.empty()) fixedBasisStr = fixedBasisStrDefault;
		LTQC::DayCountMethod fixedBasis(fixedBasisStr.c_str());

		// Tenor basis swap reference tenor
		std::string basisSwapTenorStrDefault(CurveType::_3M()->getDescription());
		std::string basisSwapTenorStr;
		permissive_extract(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, BASISSWAPTENOR), basisSwapTenorStr, basisSwapTenorStrDefault);
		if (basisSwapTenorStr.empty()) basisSwapTenorStr = basisSwapTenorStrDefault;
		LTQC::Tenor basisSwapTenor(basisSwapTenorStr.c_str());

		// tenor basis spread leg
		std::string tenorBasisSpreadTenorStr;
		bool hasTenorBasisSpreadTenor = IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, TENORBASISSPREADTENOR), tenorBasisSpreadTenorStr, basisSwapTenorStr);
		std::string tenorBasisSpreadBasisStr;
		bool hasTenorBasisSpreadBasis = IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, TENORBASISSPREADBASIS), tenorBasisSpreadBasisStr, basis);
		if (tenorBasisSpreadBasisStr.empty()) tenorBasisSpreadBasisStr = basis;
		LTQC::DayCountMethod tenorBasisSpreadBasis(tenorBasisSpreadBasisStr.c_str());
		LTQC::RollConvMethod tenorBasisSpreadRollConvention = rollConv;
		std::string tenorBasisSpreadAccrualCalendar = floatAccCalendar;


		bool hasTenorBasisSpreadLegDetails = hasTenorBasisSpreadTenor || hasTenorBasisSpreadBasis;
		if ((hasTenorBasisSpreadTenor && !hasTenorBasisSpreadBasis) || (!hasTenorBasisSpreadTenor && hasTenorBasisSpreadBasis))
			LTQC_THROW(IDeA::DataException, "Tenor Basis Spread Tenor and Tenor Basis Spread Basis must be both provided or both omitted");


		// Ccy Basis Swap Domestic Tenor
		std::string defaultDomesticTenorDefault("3M");
		std::string defaultDomesticTenor;
		IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, CCYBASISSWAPDOMESTICTENOR), defaultDomesticTenor, defaultDomesticTenorDefault);
		if (defaultDomesticTenor.empty()) defaultDomesticTenor = defaultDomesticTenorDefault;

		IDeA::DepositRateMktConvention depoCcySwapConvention(spotDays, ccy, LT::Str(index), floatBasis, LTQC::Tenor(defaultDomesticTenor), LT::Str(settleCalendar), rollConv,rollRuleConv, fixingAccCalendar, LTQC::RollConvMethod(LTQC::RollConvMethod::None));

        // Swap
        IDeA::SwapMktConvention swapConvention(spotDays, ccy, LT::Str(index), depoConvention, fixedTenor, floatTenor, fixedBasis, floatBasis, rollConv.asString(),fixedAccCalendar, floatAccCalendar, fixingCalendar, LTQC::RollConvMethod(LTQC::RollConvMethod::None).asString());

		// Ccy basis 
		std::string ccyBasisSwapLeg2Currency("USD"), ccyBasisSwapLeg2Index("LIBOR");
	
		std::string usdLiborkey = ccyBasisSwapLeg2Currency + ccyBasisSwapLeg2Index;
		ModuleStaticData::IRIndexPropertiesPtr usdIndexProperties = ModuleStaticData::getIRIndexProperties(usdLiborkey);
        ostringstream tmp;
        tmp << usdIndexProperties->getSpotDays() << "B";
		std::string ccyBasisSpotDaysStrDefault = tmp.str();
        LTQC::Tenor ccyBasisSpotDays(ccyBasisSpotDaysStrDefault.c_str());
        IDeA::CurrencyBasisSwapMktConvention ccyBasisSwapConvention(ccyBasisSpotDays, ccy, LT::Str(index), depoCcySwapConvention, depoCcySwapConvention.m_rateTenor, floatBasis, floatAccCalendar, fixingCalendar, rollConv.asString());

        // Tenor basis
        IDeA::IRRateConstPtr underlyingRate = IDeA::IRRateConstPtr( new IDeA::IRDepositRate( depoConvention.m_currency, depoConvention.m_index, basisSwapTenor, depoConvention.m_dcm, depoConvention.m_spotDays, depoConvention.m_fixingCalendar,
                                                                                             depoConvention.m_rollConvention, depoConvention.m_accrualValueCalendar, depoConvention.m_rollRuleConvention, IDeA::DepositRatePeriodCalculationType::RateTenor ) );
        IDeA::TenorBasisSwapMktConvention::ReferenceLegMktConvention referenceLegMktConvention( underlyingRate, depoConvention.m_dcm, depoConvention.m_rollConvention.asString(), depoConvention.m_rollRuleConvention, depoConvention.m_accrualValueCalendar, LTQC::RollConv(LTQC::RollConvMethod::None).asString() );
        IDeA::TenorBasisSwapMktConvention::OtherLegMktConvention otherLegMktConvention( depoConvention.m_dcm, depoConvention.m_rollConvention.asString(), depoConvention.m_rollRuleConvention, depoConvention.m_accrualValueCalendar, depoConvention.m_fixingCalendar, LTQC::RollConv(LTQC::RollConvMethod::None).asString() );

        IDeA::TenorBasisSwapMktConvention::SpreadLegMktConvention spreadLegMktConvention;
		if( hasTenorBasisSpreadLegDetails )
        {
			spreadLegMktConvention = IDeA::TenorBasisSwapMktConvention::SpreadLegMktConvention( LTQC::Tenor(tenorBasisSpreadTenorStr), tenorBasisSpreadBasis, tenorBasisSpreadRollConvention.asString(), depoConvention.m_rollRuleConvention, tenorBasisSpreadAccrualCalendar );
        }
        IDeA::TenorBasisSwapMktConvention tenorBasisSwapConvention( depoConvention.m_spotDays, depoConvention.m_currency, depoConvention.m_index, referenceLegMktConvention, otherLegMktConvention, spreadLegMktConvention );

		// FxForwards
		std::string ccypair, defaultCcyPair;
		LTQC::Currency ccyForeign = ccyBasisSwapLeg2Currency;
		defaultCcyPair = ccyForeign.string() + ccy.string();
	    bool foundFxIndex = IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FXINDEX), ccypair, defaultCcyPair);
		
        if( !foundFxIndex )
        {
            ModuleStaticData::CurrencyPairPropertiesPtr fxProps(ModuleStaticData::getCurrencyPairProperties(ccypair));
            ccypair = fxProps->getMainCurrency() + fxProps->getMoneyCurrency();
        }

        ModuleDate::FXStaticData fxStaticData;
		bool hasDefaults = ModuleDate::FXStaticData::tryGet(ccypair, fxStaticData);

		if(!hasDefaults)
		{
            // Defaults are not available, create some from the IR static data.
            fxStaticData.setExplicit(spotDays.asDays(), settleCalendar, usdIndexProperties->getCalendarName());
		}
        IDeA::FxForwardMktConvention fxForwardConvention( ccy, ccyForeign, ccypair, fxStaticData);

		// OIS
		LTQC::Tenor overnightSpotDays("0D");
		LTQC::Tenor overnightTenor("1D");
		LTQC::Tenor oisPayDelay("0D");
		LTQC::RollConvMethod overnightRollConv(LTQC::RollConvMethod::Following);
		LTQC::RollRuleMethod overnightRollRuleConv(LTQC::RollRuleMethod::None);

		IDeA::DepositRateMktConvention overnightDepoConvention(overnightSpotDays, ccy, LT::Str("Overnight"), floatBasis, overnightTenor, LT::Str(settleCalendar), overnightRollConv, overnightRollRuleConv, fixingAccCalendar, LTQC::RollConvMethod(LTQC::RollConvMethod::None));
        IDeA::OISMktConvention oisConvention(spotDays, ccy, LT::Str("OIS"), overnightDepoConvention, fixedTenor, floatBasis, rollConv.asString(), floatAccCalendar, fixingCalendar, LT::Str(settleCalendar) ,oisPayDelay, LTQC::StubType::SS);

        // OIS Swap
		IDeA::OISSwapMktConvention oisSwapConvention(oisConvention, fixedTenor, fixedBasis, fixedAccCalendar);

        // OIS Basis Swap
		IDeA::OISBasisSwapMktConvention oisBasisSwapConvention(oisConvention, depoConvention, floatTenor, floatTenor, floatBasis, floatBasis, floatAccCalendar, floatAccCalendar);
		
        IDeA::IRCurveMktConventionPtr curveConvention(new IDeA::IRCurveMktConvention(depoConvention, fraConvention, futureConvention, swapConvention, tenorBasisSwapConvention, ccyBasisSwapConvention, fxForwardConvention, oisSwapConvention,oisBasisSwapConvention));
		return curveConvention;
	}

	IDeA::IRCurveMktConventionPtr createIRCurveMktConventions(const LTQuant::GenericData& curveParametersTable)
	{
		// Build IRCurve convention
		// Take conventions from curveParametersTable or, if missing, from defaults IRIndexProperties

		const std::string currency(IDeA::extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY)));
		const std::string index(IDeA::extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX)));

		const std::string key = currency + index;
		ModuleStaticData::IRIndexPropertiesPtr indexProperties = ModuleStaticData::getIRIndexProperties(key);
		return createIRCurveMktConventions(curveParametersTable, indexProperties);
	}

    IDeA::IRCurveMktConventionPtr createIRCurveMktConventions(const LTQuant::GenericDataPtr& curveParametersTable)
    {
		return createIRCurveMktConventions(*curveParametersTable);
    }

    void fillStartAndEndDates(const LTQuant::GenericDataPtr& instrumentTable,
                              const size_t index,
                              const string& description,
							  const string& fixingCalendar,
							  const string& accrualCalendar,
							  const LTQC::Tenor& spotDays,
                              const LT::date& valueDate,
                              LT::date& startDate,
                              LT::date& endDate)
    {
        LT::Cell startDateAsCell, emptyCell;
        bool found = IDeA::permissive_extract< LT::Cell >( *(instrumentTable->table), IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE ), index, startDateAsCell, emptyCell );
        if( found )
        {
            startDate = startDateFromCell(startDateAsCell, valueDate, fixingCalendar, accrualCalendar, spotDays); 
        }
        else
        {
            startDate = IDeA::TradeConventionCalcH::getSpotDate(Date(valueDate), fixingCalendar, accrualCalendar, spotDays).getAsLTdate();
        }

		IDeA::permissive_extract<LT::date>(*instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE), index, endDate, ModuleDate::addDatePeriod(startDate, description, ModuleDate::CalendarFactory::create(accrualCalendar)));
	}

    void fillEndDate(const LTQuant::GenericDataPtr& instrumentTable,
                              const size_t index,
                              const string& description,
							  const string& accrualCalendar,
                              const LT::date& startDate,
                              LT::date& endDate)
    {
		IDeA::permissive_extract<LT::date>(*instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE), index, endDate, ModuleDate::addDatePeriod(startDate, description, ModuleDate::CalendarFactory::create(accrualCalendar)));
	}

    LT::date startDateFromCell(const LT::Cell& startDateAsCell,
                               const LT::date& buildDate,
							   const LT::Str&  fixingCalendar,
							   const LT::Str&  accCalendar,
							   const LTQC::Tenor& spotDays)
    { 
        LT::date startDate;
        if( startDateAsCell.valueType( ) != LT::Cell::double_type )
        {
             if( !startDateAsCell.empty( ) )
             {   
                  Date start = IDeA::TradeConventionCalcH::getSpotDate(Date(buildDate), fixingCalendar, accCalendar, spotDays);
                  LTQC::retrieveDateFromCell( startDateAsCell, start, accCalendar, LTQC::RollConv(LTQC::RollConvMethod::Following), LTQC::RollRuleMethod(LTQC::RollRuleMethod::None) );
                  startDate = LT::date(start);
             }
             else
             {
                LTQC_THROW( LTQC::DataQCException, "Cannot find start date empty cell passed" );
             }
        }
        else
        {
            const double startDateAsDbl = startDateAsCell;
            startDate = static_cast<long>( startDateAsDbl );
        }
        return startDate;
    }

    LT::date startDateFromDateOrTenor(const LTQuant::GenericData& instrumentParametersTable,
                                      const size_t index,
									  const LT::Ptr<LT::date>& buildDate,
									  const LT::Str& fixingCalendar,
									  const LT::Str& accCalendar,
									  const LTQC::Tenor& spotDays,
                                      LT::date& actualBuildDate,
                                      LT::date& fixingDate)
    { 
        LT::date startDate;
        if(buildDate)
		{
            LT::Cell startDateAsCell, emptyCell;
            bool found = IDeA::permissive_extract< LT::Cell >( instrumentParametersTable.table, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE ), index, startDateAsCell, emptyCell );
     
           
            if( found )
            {
               startDate = startDateFromCell(startDateAsCell, *buildDate, fixingCalendar, accCalendar, spotDays);
            }
            else
            {
				startDate = IDeA::TradeConventionCalcH::getSpotDate(Date(*buildDate), fixingCalendar, accCalendar, spotDays).getAsLTdate();
			}
	        fixingDate = *buildDate;
			actualBuildDate = *buildDate;
		}
		else
		{
			startDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE));
			actualBuildDate = fixingDate = IDeA::IRDepositRateCalcH::calculateFixingDate(Date(startDate), spotDays, fixingCalendar).getAsLTdate();
		}
        return startDate;
    }

	// TO DO: templatize on the table - requires to handle string's correctly
	void setMaturityAndDates(const LTQuant::GenericData& instrumentParametersTable,
										const LT::Ptr<LT::date>& buildDate,
										const LT::Str& fixingCalendar,
										const LT::Str& accCalendar,
										const LTQC::Tenor& spotDays,
										std::string& maturity,
										LT::date& actualBuildDate,
										LT::date& fixingDate,
										LT::date& startDate,
										LT::date& endDate)
	{
		startDate = startDateFromDateOrTenor(instrumentParametersTable, 0, buildDate, fixingCalendar, accCalendar, spotDays, actualBuildDate, fixingDate);
		// Maturity
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, YCI_MATURITY), maturity);
		
		// End Date
		if(maturity.empty())
		{
			// No maturity specified, so the end date has to be explicitly provided
			endDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE));
		}
		else
		{
			// If a maturity is provided, use it to calculate the end date from the start date the end date is not provided
			IDeA::permissive_extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE), endDate, ModuleDate::addDatePeriod(startDate, maturity, ModuleDate::CalendarFactory::create(accCalendar.data())));
		}
	}

    void setMaturityAndDates(const LTQuant::GenericData& instrumentParametersTable,
										const LT::Ptr<LT::date>& buildDate,
										const LT::Str& fixingCalendar,
										const LT::Str& accCalendar,
										const LTQC::Tenor& spotDays,
                                        const LT::date& startDate,
										std::string& maturity,
										LT::date& actualBuildDate,
										LT::date& fixingDate,
										LT::date& endDate)
	{
		if(buildDate)
		{
			fixingDate = *buildDate;
			actualBuildDate = *buildDate;
		}
		else
		{
			actualBuildDate = fixingDate = IDeA::IRDepositRateCalcH::calculateFixingDate(Date(startDate), spotDays, fixingCalendar).getAsLTdate();
		}

		// Maturity
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, YCI_MATURITY), maturity);
		
		// End Date
		if(maturity.empty())
		{
			// No maturity specified, so the end date has to be explicitly provided
			endDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE));
		}
		else
		{
			// If a maturity is provided, use it to calculate the end date from the start date the end date is not provided
			IDeA::permissive_extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE), endDate, ModuleDate::addDatePeriod(startDate, maturity, ModuleDate::CalendarFactory::create(accCalendar.data())));
		}
	}

    void setMaturityAndDates(const LT::date& buildDate, const LT::Str& fixingCalendar, const LT::Str& accCalendar, const LTQC::Tenor& spotDays, const std::string& maturity, 
        LT::date& actualBuildDate, LT::date& fixingDate, LT::date& startDate, LT::date& endDate)
	{
		startDate       = IDeA::TradeConventionCalcH::getSpotDate(Date(buildDate), fixingCalendar, accCalendar, spotDays).getAsLTdate();
		fixingDate      = buildDate;
		actualBuildDate = buildDate;
		endDate         = ModuleDate::addDatePeriod(startDate, maturity, ModuleDate::CalendarFactory::create(accCalendar.data()));
	}

	std::string getDescriptionOrEquivalent(const LTQuant::GenericDataPtr& instrumentTable,
										   const std::string& equivalentTag,
										   const size_t index)
	{
		std::string description("");

		// try to get the value for tag "Description"
		instrumentTable->permissive_get<std::string>("Description", index, description, "");

		if(description.empty())
		{
			// No "Description" tag? see if there's an equivalent tag
			instrumentTable->permissive_get<std::string>(equivalentTag, index, description, "");
		}

		return description;
	}

	double getRateOrSpread(const LTQuant::GenericDataPtr& instrumentTable,
						   const size_t index)
	{
		double rate;

		if(instrumentTable->doesTagExist("Rate"))
		{
			instrumentTable->strict_get<double>("Rate", index, rate);
		}
		else if(instrumentTable->doesTagExist("Spread"))
		{
			instrumentTable->strict_get<double>("Spread", index, rate);
		}
		else
		{
			LT_THROW_ERROR( std::string("No 'Rate' or 'Spread' tag found in table: ").append(instrumentTable->getTableName()) ); 
		}

		return rate;
	}

}