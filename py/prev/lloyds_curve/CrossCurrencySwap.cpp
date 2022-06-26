/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "CrossCurrencySwap.h"
#include "BaseModel.h"
#include "Data/GenericData.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "GlobalComponentCache.h"
#include "AllComponentsAndCaches.h"
#include "DataExtraction.h"
#include "FlexYcfUtils.h"
#include "DateUtils.h"
#include "RepFlowsData.h"
#include "StubUtils.h"
#include "FlexYCFCloneLookup.h"
#include "FXForward.h"
// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "MarketConvention.h"


using namespace LTQC;
using namespace std;
using namespace IDeA;

namespace
{
	void extractCrossCurrencySwapInputs(IDeA::IRCurveMktConventionPtr conventions, const LTQuant::GenericData& instrumentTable, size_t cnt, IDeA::CurrencyBasisSwapMktConvention& swapDetails)
	{
			std::string legType; 
            IDeA::LegType floatingLegType = IDeA::LegType::Floating;
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, DOMESTICLEGTYPE), cnt, legType, floatingLegType.asString().data());
			swapDetails.m_domLegType = IDeA::LegType(legType);
				
			std::string leg1SpotDays, leg2SpotDays, leg1FixingCal, leg2FixingCal, leg1AccrualCalendar, forCcy, forIndex, forAccrualCalendar, forFrequency, leg1Frequency, index, leg1PayDelay, leg2PayDelay, forLegType, depositRateType, leg1RollConv, leg1RollRuleConv, leg2RollConv, leg2RollRuleConv, leg2DepositRateType;
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1RATESPOTDAYS), cnt, leg1SpotDays, swapDetails.m_depRateMktConvention.m_spotDays.asString().data());
			swapDetails.m_depRateMktConvention.m_spotDays = Tenor(leg1SpotDays);

			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2RATESPOTDAYS), cnt, leg2SpotDays, swapDetails.m_forDepRateMktConvention.m_spotDays.asString().data());
			swapDetails.m_forDepRateMktConvention.m_spotDays = Tenor(leg2SpotDays);

			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1FIXINGCALENDAR), cnt, leg1FixingCal, swapDetails.m_depRateMktConvention.m_fixingCalendar.data());
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2FIXINGCALENDAR), cnt, leg2FixingCal, swapDetails.m_forDepRateMktConvention.m_fixingCalendar.data());
			swapDetails.m_depRateMktConvention.m_fixingCalendar = leg1FixingCal;
				
				
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2CURRENCY), cnt, forCcy, swapDetails.m_forCurrency.string());
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2INDEX), cnt, forIndex, swapDetails.m_forIndex.string());
			swapDetails.m_forCurrency = forCcy;
			swapDetails.m_forIndex = forIndex;

            IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2TYPE), cnt, forLegType, swapDetails.m_forLegType.asString().data());
			swapDetails.m_forLegType = IDeA::CrossCcyForeignLegType(forLegType);
                
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1ACCRUALCALENDAR), cnt, leg1AccrualCalendar, swapDetails.m_floatAccrualCalendar.data());
			swapDetails.m_floatAccrualCalendar = leg1AccrualCalendar;

			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2ACCRUALCALENDAR), cnt, forAccrualCalendar, swapDetails.m_forFloatAccrualCalendar.data());
            swapDetails.m_forFloatAccrualCalendar = forAccrualCalendar;
				
			DayCountMethod forBasis;
			IDeA::permissive_extract<DayCountMethod>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2BASIS), cnt, forBasis, swapDetails.m_forFloatAccrualBasis);
			swapDetails.m_forFloatAccrualBasis = forBasis;
				
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2FREQUENCY), cnt, forFrequency, swapDetails.m_forFloatFrequency.asString().data());
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, DOMESTICTENOR), cnt, leg1Frequency, swapDetails.m_floatFrequency.asString().data());
            bool leg1IndexProvided = IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1INDEX), cnt, index, swapDetails.m_index.string());
			swapDetails.m_forFloatFrequency = LTQC::Tenor(forFrequency);
			swapDetails.m_floatFrequency = LTQC::Tenor(leg1Frequency);
			swapDetails.m_depRateMktConvention.m_rateTenor = swapDetails.m_floatFrequency;
			swapDetails.m_index = index;
			if(leg1IndexProvided)
			{
				swapDetails.m_depRateMktConvention.m_index = index;
			}

			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1DEPOSITRATETYPE), cnt, depositRateType, swapDetails.m_depRateMktConvention.m_depositRateType.asString().data());
            swapDetails.m_depRateMktConvention.m_depositRateType = DepositRateType(depositRateType);
				
            IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1PAYDELAY), cnt, leg1PayDelay, swapDetails.m_domLegPayDelay.data());
            swapDetails.m_domLegPayDelay = leg1PayDelay;
				
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1ROLLCONVENTION), cnt, leg1RollConv, swapDetails.m_depRateMktConvention.m_rollConvention.asString().data());
			swapDetails.m_depRateMktConvention.m_rollConvention = RollConvMethod(leg1RollConv);

			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1ROLLRULECONVENTION), cnt, leg1RollRuleConv, swapDetails.m_depRateMktConvention.m_rollRuleConvention.asString().data());
			swapDetails.m_depRateMktConvention.m_rollRuleConvention = RollRuleMethod(leg1RollRuleConv);

            DayCountMethod leg1Basis;
			IDeA::permissive_extract<DayCountMethod>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1BASIS), cnt, leg1Basis, swapDetails.m_floatAccrualBasis);
            swapDetails.m_floatAccrualBasis =leg1Basis;
			swapDetails.m_depRateMktConvention.m_dcm = leg1Basis;
				
			if(  swapDetails.m_currency.compareCaseless(swapDetails.m_forCurrency) == 0 && swapDetails.m_forLegType == IDeA::CrossCcyForeignLegType::LiborFlat)
            {
                swapDetails.m_forDepRateMktConvention = swapDetails.m_depRateMktConvention;
                swapDetails.m_spotDays = conventions->m_tenorBasisSwaps.m_spotDays;
            }
			swapDetails.m_forDepRateMktConvention.m_rateTenor = swapDetails.m_forFloatFrequency;
			swapDetails.m_forDepRateMktConvention.m_dcm = forBasis;
			swapDetails.m_forDepRateMktConvention.m_fixingCalendar = leg2FixingCal;

			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2PAYDELAY), cnt, leg2PayDelay, swapDetails.m_forLegPayDelay.data());
            swapDetails.m_forLegPayDelay = leg2PayDelay;

			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2ROLLCONVENTION), cnt, leg2RollConv, swapDetails.m_forDepRateMktConvention.m_rollConvention.asString().data());
			swapDetails.m_forDepRateMktConvention.m_rollConvention = RollConvMethod(leg2RollConv);

			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2ROLLRULECONVENTION), cnt, leg2RollRuleConv, swapDetails.m_forDepRateMktConvention.m_rollRuleConvention.asString().data());
            swapDetails.m_forDepRateMktConvention.m_rollRuleConvention = RollRuleMethod(leg2RollRuleConv);
				
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2DEPOSITRATETYPE), cnt, leg2DepositRateType, swapDetails.m_forDepRateMktConvention.m_depositRateType.asString().data());
            swapDetails.m_forDepRateMktConvention.m_depositRateType = DepositRateType(leg2DepositRateType);

			std::string leg1PeriodCalculationTypeStr, leg2PeriodCalculationTypeStr;
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG1ENDDATECALCULATIONTYPE), cnt, leg1PeriodCalculationTypeStr, swapDetails.m_depRateMktConvention.m_endDateCalculationType.asString().data());
            swapDetails.m_depRateMktConvention.m_endDateCalculationType = FlexYCF::EndDateCalculationType(leg1PeriodCalculationTypeStr);
               
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, LEG2ENDDATECALCULATIONTYPE), cnt, leg2PeriodCalculationTypeStr, swapDetails.m_forDepRateMktConvention.m_endDateCalculationType.asString().data());
            swapDetails.m_forDepRateMktConvention.m_endDateCalculationType = FlexYCF::EndDateCalculationType(leg2PeriodCalculationTypeStr);
              
			std::string spotDays, spotCalendar;
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, SPOTDAYS), cnt, spotDays, swapDetails.m_spotDays.asString().data());
			IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(CCYBASIS, SPOTCALENDAR), cnt, spotCalendar, swapDetails.m_fixingCalendar.string());
			swapDetails.m_spotDays = Tenor(spotDays);
			swapDetails.m_fixingCalendar = spotCalendar;
	}


	FlexYCF::FloatingLegPtr createFloatingLeg(const LT::Str& maturity,
											  const LT::date valueDate,
											  const LT::date fixingDate,
											  const LT::date startDate,
											  const LT::date endDate,
											  const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
											  const FlexYCF::EndDateCalculationType applicableBackStubEndDateCalculationType,
											  FlexYCF::GlobalComponentCache* const globalComponentCache = 0)
	{
		using namespace FlexYCF;

		//	By default, do NOT adjust the calculation of the end date for the last period of the floating leg
		EndDateCalculationType backStubEndDateCalculationType(EndDateCalculationType::NotAdjusted);
		
		const LTQC::Tenor maturityTenor(maturity);
		const LTQC::Tenor domesticLegTenor(tradeDetails.m_floatFrequency.asTenorString());

		//	Checks if there is stub period. This works for domestic leg tenors of at least 1M
		if(maturityTenor < domesticLegTenor || maturityTenor.asMonths() % domesticLegTenor.asMonths() != 0)
		{
			backStubEndDateCalculationType = applicableBackStubEndDateCalculationType;
		}

		const FloatingLegArguments fltLegArgs(valueDate,
											  fixingDate,
											  startDate,
											  endDate,
											  tradeDetails.m_floatFrequency.asTenorString().string(),
											  tradeDetails.m_floatAccrualCalendar.string(),
											  tradeDetails.m_depRateMktConvention,
											  backStubEndDateCalculationType,
											  tradeDetails.m_domLegPayDelay,
											  LTQC::DayCount::create(tradeDetails.m_floatAccrualBasis));

		return (globalComponentCache? globalComponentCache->get(fltLegArgs): FloatingLeg::create(fltLegArgs));
	}
    
    FlexYCF::FloatingLegPtr createForeignFloatingLeg(const LT::Str& maturity,
											  const LT::date valueDate,
											  const LT::date fixingDate,
											  const LT::date startDate,
											  const LT::date endDate,
											  const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
											  const FlexYCF::EndDateCalculationType applicableBackStubEndDateCalculationType,
											  FlexYCF::GlobalComponentCache* const globalComponentCache = 0)
	{
		using namespace FlexYCF;
        if( tradeDetails.m_forLegType == IDeA::CrossCcyForeignLegType::None || tradeDetails.m_forLegType == IDeA::CrossCcyForeignLegType::Resettable )
        {
            return FlexYCF::FloatingLegPtr();
        }
       

		//	By default, do NOT adjust the calculation of the end date for the last period of the floating leg
		EndDateCalculationType backStubEndDateCalculationType(EndDateCalculationType::NotAdjusted);
		const LTQC::Tenor maturityTenor(maturity);
		const LTQC::Tenor forLegTenor(tradeDetails.m_forFloatFrequency.asTenorString());

		//	Checks if there is stub period. This works for domestic leg tenors of at least 1M
		if(maturityTenor < forLegTenor || maturityTenor.asMonths() % forLegTenor.asMonths() != 0)
		{
			backStubEndDateCalculationType = applicableBackStubEndDateCalculationType;
		}
        
        IDeA::DepositRateMktConvention depMktConv = tradeDetails.m_forDepRateMktConvention;
        depMktConv.m_currency = tradeDetails.m_forCurrency;
        depMktConv.m_index    = tradeDetails.m_forIndex;
        depMktConv.m_dcm      = tradeDetails.m_forFloatAccrualBasis;
        depMktConv.m_rateTenor= forLegTenor;
        	
        const FloatingLegArguments fltLegArgs(valueDate,
											  fixingDate,
											  startDate,
											  endDate,
											  forLegTenor.asTenorString().string(),
											  tradeDetails.m_forFloatAccrualCalendar.string(),
											  depMktConv,
											  backStubEndDateCalculationType,
											  tradeDetails.m_forLegPayDelay,
											  LTQC::DayCount::create(tradeDetails.m_forFloatAccrualBasis));

		return (globalComponentCache? globalComponentCache->get(fltLegArgs): FloatingLeg::create(fltLegArgs));
	}

    FlexYCF::MultiCcyFloatingLegPtr createResettableForeignFloatingLeg(const LT::Str& maturity,
											  const LT::date spotFxDate,
											  const LT::date valueDate,
											  const LT::date fixingDate,
											  const LT::date startDate,
											  const LT::date endDate,
											  const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
											  const FlexYCF::EndDateCalculationType applicableBackStubEndDateCalculationType,
											  FlexYCF::GlobalComponentCache* const globalComponentCache = 0)
	{
		using namespace FlexYCF;
        if( tradeDetails.m_forLegType == IDeA::CrossCcyForeignLegType::None || tradeDetails.m_forLegType == IDeA::CrossCcyForeignLegType::LiborFlat)
        {
            return FlexYCF::MultiCcyFloatingLegPtr();
        }
       

		//	By default, do NOT adjust the calculation of the end date for the last period of the floating leg
		EndDateCalculationType backStubEndDateCalculationType(EndDateCalculationType::NotAdjusted);
		
		const LTQC::Tenor maturityTenor(maturity);
		const LTQC::Tenor forLegTenor(tradeDetails.m_forFloatFrequency.asTenorString());

		//	Checks if there is stub period. This works for domestic leg tenors of at least 1M
		if(maturityTenor < forLegTenor || maturityTenor.asMonths() % forLegTenor.asMonths() != 0)
		{
			backStubEndDateCalculationType = applicableBackStubEndDateCalculationType;
		}
        
        IDeA::DepositRateMktConvention depMktConv = tradeDetails.m_forDepRateMktConvention;
        depMktConv.m_currency = tradeDetails.m_forCurrency;
        depMktConv.m_index    = tradeDetails.m_forIndex;
        depMktConv.m_dcm      = tradeDetails.m_forFloatAccrualBasis;
        depMktConv.m_rateTenor= forLegTenor;
        depMktConv.m_spotDays = tradeDetails.m_spotDays;

        const FloatingLegArguments fltLegArgs(valueDate,
											  spotFxDate,
											  fixingDate,
											  startDate,
											  endDate,
											  forLegTenor.asTenorString().string(),
											  tradeDetails.m_forFloatAccrualCalendar.string(),
											  depMktConv,
											  backStubEndDateCalculationType);

    
        return MultiCcyFloatingLeg::create(fltLegArgs);
	}
	template<class T>
	FlexYCF::EndDateCalculationType::Enum_t extractApplicableBackStubEndDateCalculationType(const T& curveParametersTable)
	{
		using namespace FlexYCF;

		//	Extract the applicable back stub end date calculation type:
		std::string endDateCalcTypeStr;
		IDeA::permissive_extract<std::string>(curveParametersTable,
											  IDeA_KEY(YC_CURVEPARAMETERS, CCYBASISBACKSTUBENDDATE),
											  endDateCalcTypeStr,
											  EndDateCalculationType::toString(CrossCurrencySwap::defaultBackStubEndDataCalculationType()).data());
		const EndDateCalculationType applicableBackStubEndDateCalculationType(EndDateCalculationType::fromString(endDateCalcTypeStr));
		return applicableBackStubEndDateCalculationType;
	}
}


namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<CrossCurrencySwap>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, CCYBASIS);
	}

    using namespace LTQuant;

    CrossCurrencySwap::CrossCurrencySwap(const string& description,
                                         const LT::date fixingDate,
                                         const LT::date startDate,
                                         const LT::date endDate,
                                         const double spread,
										 const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
										 const LT::date valueDate,
										 const EndDateCalculationType backStubEndDateCalculationType) :
		CalibrationInstrument(spread, getKeyName<CrossCurrencySwap>(), description, fixingDate, startDate, endDate),
        m_domesticLegTenor(ModuleDate::dateDescToYears(tradeDetails.m_floatFrequency.asTenorString().data())),
		m_floatingLeg(::createFloatingLeg(description, valueDate, fixingDate, startDate, endDate, tradeDetails, backStubEndDateCalculationType)),
		m_fixedLeg(FixedLeg::create(FixedLegArguments(valueDate,
                                                      fixingDate,
                                                      startDate,
                                                      endDate,
                                                      tradeDetails.m_floatFrequency.asTenorString().string(),
                                                      tradeDetails.m_depRateMktConvention.m_dcm.asString().data(),
													  tradeDetails.m_floatAccrualCalendar.string(),
                                                      tradeDetails.m_currency,
                                                      tradeDetails.m_index))),
		m_firstDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate,startDate,tradeDetails.m_currency,tradeDetails.m_index))),
        m_lastDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate,endDate,tradeDetails.m_currency,tradeDetails.m_index))),
		m_details(tradeDetails),
        m_foreignLeg(::createForeignFloatingLeg(description, valueDate, fixingDate, startDate, endDate, tradeDetails, backStubEndDateCalculationType)),
        m_resettableForeignLeg(::createResettableForeignFloatingLeg(description, static_cast<long>(ModuleDate::getFXSpotDate(Date(valueDate), string(tradeDetails.m_forCurrency.string() + tradeDetails.m_currency.string()).c_str())), valueDate, fixingDate, startDate, endDate, tradeDetails, backStubEndDateCalculationType)),
        m_firstForeignDiscountFactor(DiscountFactor::create(DiscountFactorArguments( valueDate,startDate, tradeDetails.m_forCurrency,tradeDetails.m_forIndex))),
        m_lastForeignDiscountFactor(DiscountFactor::create(DiscountFactorArguments( valueDate, endDate, tradeDetails.m_forCurrency,tradeDetails.m_forIndex ))),
        m_isInitialized(false),
        m_singleCurrencyBasisSwap(tradeDetails.m_currency.compareCaseless(tradeDetails.m_forCurrency) == 0 && tradeDetails.m_forLegType == IDeA::CrossCcyForeignLegType::LiborFlat)
    {
    }
	
    CrossCurrencySwap::CrossCurrencySwap(const string& description,
                                         const LT::date fixingDate,
                                         const LT::date leg1StartDate,
                                         const LT::date leg1EndDate,
                                         const LT::date leg2StartDate,
                                         const LT::date leg2EndDate,
                                         const double spread,
										 const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
                                         const LT::date valueDate,
										 const EndDateCalculationType backStubEndDateCalculationType,
                                         GlobalComponentCache& globalComponentCache):
        CalibrationInstrument(spread, getKeyName<CrossCurrencySwap>(), description, fixingDate, min(leg1StartDate,leg2StartDate), max(leg1EndDate,leg2EndDate)),
		m_domesticLegTenor(ModuleDate::dateDescToYears(tradeDetails.m_floatFrequency.asTenorString().data())),
		m_floatingLeg(::createFloatingLeg(description, valueDate, fixingDate, leg1StartDate, leg1EndDate, tradeDetails, backStubEndDateCalculationType, &globalComponentCache)),
		m_fixedLeg(globalComponentCache.get( FixedLeg::Arguments(valueDate,
																  fixingDate,
																  leg1StartDate,
																  leg1EndDate,
																  tradeDetails.m_floatFrequency.asTenorString().string(),
																  tradeDetails.m_depRateMktConvention.m_dcm.asString().data(),
																  tradeDetails.m_floatAccrualCalendar.string(),
                                                                  tradeDetails.m_currency,
                                                                  tradeDetails.m_index))),
        m_firstDiscountFactor(globalComponentCache.get( DiscountFactor::Arguments( valueDate,leg1StartDate,tradeDetails.m_currency,tradeDetails.m_index))),
        m_lastDiscountFactor(globalComponentCache.get( DiscountFactor::Arguments( valueDate, leg1EndDate,tradeDetails.m_currency,tradeDetails.m_index))),
		m_details(tradeDetails),
        m_foreignLeg(::createForeignFloatingLeg(description, valueDate, fixingDate, leg2StartDate, leg2EndDate, tradeDetails, backStubEndDateCalculationType, &globalComponentCache)),
        m_resettableForeignLeg(::createResettableForeignFloatingLeg(description, static_cast<long>(ModuleDate::getFXSpotDate(Date(valueDate), string(tradeDetails.m_forCurrency.string() + tradeDetails.m_currency.string()).c_str())), valueDate, fixingDate, leg2StartDate, leg2EndDate, tradeDetails, backStubEndDateCalculationType, &globalComponentCache)),
        m_firstForeignDiscountFactor(globalComponentCache.get( DiscountFactor::Arguments( valueDate,leg2StartDate, tradeDetails.m_forCurrency,tradeDetails.m_forIndex))),
        m_lastForeignDiscountFactor(globalComponentCache.get( DiscountFactor::Arguments( valueDate, leg2EndDate, tradeDetails.m_forCurrency, tradeDetails.m_forIndex))),
        m_isInitialized(false),
        m_singleCurrencyBasisSwap(tradeDetails.m_currency.compareCaseless(tradeDetails.m_forCurrency) == 0 && tradeDetails.m_forLegType == IDeA::CrossCcyForeignLegType::LiborFlat)
    {
    }

	CalibrationInstrumentPtr CrossCurrencySwap::create(const LTQuant::GenericData& instrumentParametersTable, 
													   const LT::Ptr<LT::date>& buildDate,
													   const LTQuant::GenericData& curveParametersTable,
													   const LTQuant::GenericDataPtr&)
	{
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(curveParametersTable));		
		IDeA::CurrencyBasisSwapMktConvention& swapDetails = conventions->m_currencyBasisSwaps;	
		extractCrossCurrencySwapInputs(conventions, instrumentParametersTable, 0, swapDetails);
		std::string maturity;
		LT::date fixingDate, actualBuildDate, startDate, endDate;
		setMaturityAndDates(instrumentParametersTable, buildDate, swapDetails.m_fixingCalendar, swapDetails.m_depRateMktConvention.m_accrualValueCalendar, swapDetails.m_spotDays, maturity, actualBuildDate, fixingDate, startDate, endDate);
		
		//	Extract the applicable back stub end date calculation type:
		const EndDateCalculationType applicableBackStubEndDateCalculationType(extractApplicableBackStubEndDateCalculationType(curveParametersTable));

		return CalibrationInstrumentPtr(new CrossCurrencySwap(maturity, fixingDate, startDate, endDate, 0.0, swapDetails, actualBuildDate, applicableBackStubEndDateCalculationType));
	}

    void CrossCurrencySwap::createInstruments(CalibrationInstruments& instruments, 
                                              LTQuant::GenericDataPtr instrumentTable,
                                              LTQuant::GenericDataPtr masterTable,
                                              GlobalComponentCache& globalComponentCache,
                                              const LTQuant::PriceSupplierPtr)
    {
		const size_t nbCcySwaps(IDeA::numberOfRecords(*instrumentTable));

        // just finish if we have empty table or just headings
        if(nbCcySwaps == 0)
        {
            return;
        }

		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		const LT::date fixingDate(valueDate);

        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));		
		
		
		//	Extract the applicable back stub end date calculation type:
		const EndDateCalculationType applicableBackStubEndDateCalculationType(extractApplicableBackStubEndDateCalculationType(parametersTable));
		
        for(size_t cnt(0); cnt < nbCcySwaps; ++cnt)
        {
			 //	Description/Tenor (e.g. 5Y)
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(CCYBASIS, TENOR), cnt));

			if(!description.empty()) // skip empty rows
            {
				// get rate
                const double rate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(CCYBASIS, SPREAD), cnt));
                IDeA::CurrencyBasisSwapMktConvention swapDetails(conventions->m_currencyBasisSwaps);

				extractCrossCurrencySwapInputs(conventions, *instrumentTable, cnt, swapDetails);
                
				// calc dates
                LT::date leg1StartDate, leg1EndDate, leg2StartDate, leg2EndDate;
                fillStartAndEndDates(instrumentTable, cnt, description, swapDetails.m_fixingCalendar.string(), swapDetails.m_floatAccrualCalendar.string(), swapDetails.m_spotDays, fixingDate, leg1StartDate, leg1EndDate);
				fillStartAndEndDates(instrumentTable, cnt, description, swapDetails.m_fixingCalendar.string(), swapDetails.m_forFloatAccrualCalendar.string(), swapDetails.m_spotDays, fixingDate, leg2StartDate, leg2EndDate);
				
				if( swapDetails.m_forLegType == IDeA::CrossCcyForeignLegType::None )
				{
					leg2StartDate  = leg1StartDate;
					leg2EndDate    = leg1EndDate;
				}
				// build instrument
                CalibrationInstrumentPtr instrument( new CrossCurrencySwap( description,
                                                                            fixingDate,
                                                                            leg1StartDate,
                                                                            leg1EndDate,
                                                                            leg2StartDate,
                                                                            leg2EndDate,
                                                                            rate,
                                                                            swapDetails,
                                                                            valueDate,
																			applicableBackStubEndDateCalculationType,
                                                                            globalComponentCache)
                                                   );
                // add
				instruments.add(instrument);
            }
        }
    }
    
    void CrossCurrencySwap::updateInstruments(CalibrationInstruments& instrumentList, 
                                              LTQuant::GenericDataPtr instrumentTable, 
                                              size_t* instrumentIndex)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentTable));
        for(size_t i(0); i < nbInstruments; ++i)
        {
			//	Description/Tenor (e.g. 5Y)
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(CCYBASIS, TENOR), i));
            if(!description.empty())
            {
                const double rate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(CCYBASIS, SPREAD), i));
                instrumentList[*instrumentIndex]->setRate(rate);
                ++(*instrumentIndex);
            }
        }
    }

    const double CrossCurrencySwap::computeModelPrice(const BaseModelPtr model) const
    {
        double domPV = 0.0, forPV = 0.0;
        if( m_singleCurrencyBasisSwap )
        {
            domPV = m_floatingLeg->getValue(*model) + getRate() * m_fixedLeg->getValue(*model);
            if( m_foreignLeg )
            {
                initializeForeignLegPricing(model);
                forPV = m_foreignLeg->getValue(*m_foreignModel);
            }
            return domPV - forPV; 
        }

		if( domesticLegType() == IDeA::LegType::Floating )
		{
			domPV = m_floatingLeg->getValue(*model) + getRate() * m_fixedLeg->getValue(*model) + m_lastDiscountFactor->getValue(*model) - m_firstDiscountFactor->getValue(*model);
		}
		else
		{
			domPV = getRate() * m_fixedLeg->getValue(*model) + m_lastDiscountFactor->getValue(*model) - m_firstDiscountFactor->getValue(*model);
		}
        
        if( m_foreignLeg )
        {
            initializeForeignLegPricing(model);
            forPV = fxRate(model) * (m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel)) / m_fxSpot;
            return domPV - forPV;
        }

         if( m_resettableForeignLeg )
        {
            initializeForeignLegPricing(model);
            double fx = fxRate(model);
            forPV = fx * m_resettableForeignLeg->getValue(*m_foreignModel, *model, m_fxSpot);
            return domPV - forPV;
        }
        return domPV;
    }
        
    void CrossCurrencySwap::accumulateGradient(BaseModel const& baseModel,
                                               double multiplier,
                                               GradientIterator gradientBegin,
                                               GradientIterator gradientEnd)
    {
        if( m_singleCurrencyBasisSwap )
        {
            m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd);
            m_floatingLeg->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
            return;
        }

		if( domesticLegType() == IDeA::LegType::Floating )
		{
			// init the gradient argument with the gradient of the floating leg
			m_floatingLeg->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
		}
        
		if( m_foreignLeg )
        {
            initializeForeignLegPricing(baseModel);
            double forPV = fxRate(baseModel) * (m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel)) / m_fxSpot;
            m_fxSpotDomDiscountFactor->accumulateGradient(baseModel, -multiplier * forPV/m_fxSpotDomDiscountFactor->getValue(baseModel) , gradientBegin, gradientEnd);
        }

        if( m_resettableForeignLeg )
        {
            initializeForeignLegPricing(baseModel);
            double fx = fxRate(baseModel);
            m_fxSpotDomDiscountFactor->accumulateGradient(baseModel, -multiplier * m_fxSpot/m_fxSpotForDiscountFactor->getValue(*m_foreignModel) * m_resettableForeignLeg->getValue(*m_foreignModel, baseModel, m_fxSpot), gradientBegin, gradientEnd);
            m_resettableForeignLeg->accumulateGradient(*m_foreignModel, baseModel, m_fxSpot, -multiplier * fx, gradientBegin, gradientEnd);
        }
        
        // compute the fixed leg gradient, multiply it by the basis spread, and add it to the gradient argument
        m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd);

        // similarly, compute the gradient of the discount factors, subtract the "first" and add the "last" to the gradient argument
        m_firstDiscountFactor->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd);

        m_lastDiscountFactor->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
    }

	void CrossCurrencySwap::accumulateGradient(BaseModel const& baseModel,
											   double multiplier,
											   GradientIterator gradientBegin,
											   GradientIterator gradientEnd,
											   const CurveTypeConstPtr& curveType)
	{

        if( m_singleCurrencyBasisSwap )
        {
            m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd,curveType);
            m_floatingLeg->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd,curveType);
            return;
        }

		if( domesticLegType() == IDeA::LegType::Floating )
		{
			// init the gradient argument with the gradient of the floating leg
			m_floatingLeg->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);
		}
        
		if( m_foreignLeg )
        {
            initializeForeignLegPricing(baseModel);
            double forPV = fxRate(baseModel) * (m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel)) / m_fxSpot;
            m_fxSpotDomDiscountFactor->accumulateGradient(baseModel, -multiplier * forPV/m_fxSpotDomDiscountFactor->getValue(baseModel) , gradientBegin, gradientEnd, curveType);
        }

        if( m_resettableForeignLeg )
        {
            initializeForeignLegPricing(baseModel);
            double fx = fxRate(baseModel);
            m_fxSpotDomDiscountFactor->accumulateGradient(baseModel, -multiplier * m_fxSpot/m_fxSpotForDiscountFactor->getValue(*m_foreignModel) * m_resettableForeignLeg->getValue(*m_foreignModel, baseModel, m_fxSpot), gradientBegin, gradientEnd, curveType);
            m_resettableForeignLeg->accumulateGradient(*m_foreignModel, baseModel, m_fxSpot, -multiplier * fx, gradientBegin,gradientEnd,curveType);

        }

        // compute the fixed leg gradient, multiply it by the basis spread, and add it to the gradient argument
        m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd, curveType);

        // similarly, compute the gradient of the discount factors, subtract the "first" and add the "last" to the gradient argument
        m_firstDiscountFactor->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd, curveType);

        m_lastDiscountFactor->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);	
	}
    void CrossCurrencySwap::accumulateGradientDependentModel(BaseModel const& modelFor, BaseModel const& modelDom,
                                               double multiplier,
                                               GradientIterator gradientBegin,
                                               GradientIterator gradientEnd)
    {
        
		if( m_foreignLeg )
        {
			double domDf = m_fxSpotDomDiscountFactor->getValue(modelDom);
			double forDf = m_fxSpotForDiscountFactor->getValue(modelFor);

			double fx = domDf/forDf;
			m_firstForeignDiscountFactor->accumulateGradient(modelFor, multiplier * fx, gradientBegin, gradientEnd);
			m_lastForeignDiscountFactor->accumulateGradient(modelFor, -multiplier * fx, gradientBegin, gradientEnd);
			m_foreignLeg->accumulateGradient(modelFor,-multiplier * fx, gradientBegin, gradientEnd);

			double pv = m_foreignLeg->getValue(modelFor) + m_lastForeignDiscountFactor->getValue(modelFor) - m_firstForeignDiscountFactor->getValue(modelFor);
			m_fxSpotForDiscountFactor->accumulateGradient(modelFor, multiplier * pv * domDf / (forDf * forDf), gradientBegin, gradientEnd);
        }
		if( m_resettableForeignLeg )
        {
			double fx = fxRate(modelDom);
			double pv = m_resettableForeignLeg->getValue(modelFor, modelDom, m_fxSpot);
          
            m_resettableForeignLeg->accumulateGradientConstantDiscountFactor(modelFor, modelDom, - multiplier * fx,  m_fxSpot,gradientBegin, gradientEnd,false);
			double domDf = m_fxSpotDomDiscountFactor->getValue(modelDom);
			double forDf = m_fxSpotForDiscountFactor->getValue(modelFor);
			m_fxSpotForDiscountFactor->accumulateGradient(modelFor, multiplier * pv * fx / forDf, gradientBegin, gradientEnd);
        }
    }

	
	void CrossCurrencySwap::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                               double multiplier,
                                               GradientIterator gradientBegin,
                                               GradientIterator gradientEnd,
											   bool spread)
    {	
		size_t k = baseModel.numberOfPlacedInstruments();
        if( m_singleCurrencyBasisSwap )
        {
            m_floatingLeg->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier, gradientEnd - k, gradientEnd,spread);
			m_fixedLeg->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier * getRate(), gradientEnd - k, gradientEnd,spread);
			if( m_foreignLeg )
            {
                m_foreignLeg->accumulateGradient(baseModel, -multiplier, gradientEnd - k, gradientEnd);
            }
            return;
        }
		
		m_firstDiscountFactor->accumulateGradientConstantDiscountFactor(baseModel, dfModel, -multiplier, gradientEnd - k, gradientEnd, spread);
		m_lastDiscountFactor->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier, gradientEnd - k, gradientEnd, spread);	
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			m_floatingLeg->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier, gradientEnd - k, gradientEnd,spread);
			m_fixedLeg->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier * getRate(), gradientEnd - k, gradientEnd,spread);
		}
		else
		{
			m_fixedLeg->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier * getRate(), gradientEnd - k, gradientEnd,spread);
		}
		 
		if( m_foreignLeg )
        {
			initializeForeignLegPricing(dfModel);
		
			double domDf = m_fxSpotDomDiscountFactor->getValue(dfModel);
			double forDf = m_fxSpotForDiscountFactor->getValue(*m_foreignModel);
			size_t k1 = m_foreignModel->numberOfPlacedInstruments();
			double fx = domDf/forDf;
			m_firstForeignDiscountFactor->accumulateGradient(*m_foreignModel, multiplier * fx, gradientBegin, gradientBegin + k1);
			m_lastForeignDiscountFactor->accumulateGradient(*m_foreignModel, -multiplier * fx, gradientBegin, gradientBegin + k1);
			m_foreignLeg->accumulateGradient(*m_foreignModel,-multiplier * fx, gradientBegin, gradientBegin + k);

			double pv = m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel);
			
			m_fxSpotDomDiscountFactor->accumulateGradientConstantDiscountFactor(baseModel, dfModel, - multiplier * pv / forDf, gradientEnd - k, gradientEnd,spread);
			m_fxSpotForDiscountFactor->accumulateGradient(*m_foreignModel, multiplier * pv * domDf / (forDf * forDf), gradientBegin, gradientBegin + k1);
        }
		 
		if( m_resettableForeignLeg )
        {
			initializeForeignLegPricing(dfModel);
			
			double domDf = m_fxSpotDomDiscountFactor->getValue(dfModel);
			double forDf = m_fxSpotForDiscountFactor->getValue(*m_foreignModel);
			size_t k1 = m_foreignModel->numberOfPlacedInstruments();
            double fx = fxRate(dfModel);
			double pv = m_resettableForeignLeg->getValue(*m_foreignModel, dfModel, m_fxSpot);

            m_fxSpotDomDiscountFactor->accumulateGradientConstantDiscountFactor(baseModel, dfModel, - multiplier * pv * m_fxSpot / forDf, gradientEnd - k, gradientEnd,spread);
			m_fxSpotForDiscountFactor->accumulateGradient(*m_foreignModel, multiplier * pv * domDf * m_fxSpot / (forDf * forDf), gradientBegin, gradientBegin + k1);
            m_resettableForeignLeg->accumulateGradientConstantDiscountFactor(*m_foreignModel, dfModel, - multiplier * fx, m_fxSpot, gradientBegin, gradientBegin + k1,false);
			if(spread)
			{
				m_resettableForeignLeg->accumulateGradientConstantDiscountFactor(*m_foreignModel, dfModel, - multiplier * fx, m_fxSpot, gradientBegin, gradientBegin + k1,true);
			}
		}
    }
    
	void CrossCurrencySwap::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                               double multiplier,
                                               GradientIterator gradientBegin,
                                               GradientIterator gradientEnd,
											   bool spread)
    {	
		size_t k = baseModel.numberOfPlacedInstruments();
        if( m_singleCurrencyBasisSwap )
        {
			m_floatingLeg->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier, gradientEnd - k, gradientEnd,spread);
			m_fixedLeg->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier * getRate(), gradientEnd - k, gradientEnd,spread);
			if( m_foreignLeg )
            {
                m_foreignLeg->accumulateGradient(baseModel, -multiplier, gradientEnd - k, gradientEnd);
            }
            return;
        }
		
		m_firstDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, -multiplier, gradientEnd - k, gradientEnd, spread);
		m_lastDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier, gradientEnd - k, gradientEnd, spread);	
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			m_floatingLeg->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier, gradientEnd - k, gradientEnd,spread);
			m_fixedLeg->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier * getRate(), gradientEnd - k, gradientEnd,spread);
		}
		else
		{
			m_fixedLeg->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier * getRate(), gradientEnd - k, gradientEnd,spread);
		}
		 
		if( m_foreignLeg )
        {
			initializeForeignLegPricing(dfModel);
		
			double domDf = m_fxSpotDomDiscountFactor->getValue(dfModel);
			double forDf = m_fxSpotForDiscountFactor->getValue(*m_foreignModel);
			size_t k1 = m_foreignModel->numberOfPlacedInstruments();
			double fx = domDf/forDf;
			m_firstForeignDiscountFactor->accumulateGradient(*m_foreignModel, multiplier * fx, gradientBegin, gradientBegin + k1);
			m_lastForeignDiscountFactor->accumulateGradient(*m_foreignModel, -multiplier * fx, gradientBegin, gradientBegin + k1);
			m_foreignLeg->accumulateGradient(*m_foreignModel,-multiplier * fx, gradientBegin, gradientBegin + k1);

			double pv = m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel);
			
			m_fxSpotDomDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, - multiplier * pv / forDf, gradientEnd - k, gradientEnd,spread);
			m_fxSpotForDiscountFactor->accumulateGradient(*m_foreignModel, multiplier * pv * domDf / (forDf * forDf), gradientBegin, gradientBegin + k1);
        }
		 
		if( m_resettableForeignLeg )
        {
			initializeForeignLegPricing(dfModel);
			
			double domDf = m_fxSpotDomDiscountFactor->getValue(dfModel);
			double forDf = m_fxSpotForDiscountFactor->getValue(*m_foreignModel);
			size_t k1 = m_foreignModel->numberOfPlacedInstruments();
            double fx = fxRate(dfModel);
			double pv = m_resettableForeignLeg->getValue(*m_foreignModel, dfModel, m_fxSpot);

            m_fxSpotDomDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, - multiplier * pv * m_fxSpot / forDf, gradientEnd - k, gradientEnd,spread);
			m_fxSpotForDiscountFactor->accumulateGradient(*m_foreignModel, multiplier * pv * domDf * m_fxSpot / (forDf * forDf), gradientBegin, gradientBegin + k1);
            m_resettableForeignLeg->accumulateGradientConstantDiscountFactor(*m_foreignModel, dfModel, - multiplier * fx, m_fxSpot, gradientBegin, gradientBegin + k1,false);
		}
    }

	void CrossCurrencySwap::update()
    {
        m_floatingLeg->update();
        m_fixedLeg->update();
        m_firstDiscountFactor->update();
        m_lastDiscountFactor->update();
        if( m_foreignLeg )
        {
            m_foreignLeg->update();
            m_firstForeignDiscountFactor->update();
            m_lastForeignDiscountFactor->update();
            if( m_fxSpotDomDiscountFactor )
                m_fxSpotDomDiscountFactor->update();
            if( m_fxSpotForDiscountFactor )
                m_fxSpotForDiscountFactor->update();
        }
        if( m_resettableForeignLeg )
        {
            m_resettableForeignLeg->update();
        }
    }

	double CrossCurrencySwap::getVariableInitialGuess(const double flowTime,
													  const BaseModel* const model) const
	{
		LT_LOG << "initial guess for X" << getDescription().string()<< ", " << CurveType::Discount()->getDescription() << std::endl;
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			return model->getVariableValueFromSpineDiscountFactor(flowTime, pow(1.0 - getRate(), flowTime), CurveType::Discount());
		}
		else
		{
			return model->getVariableValueFromSpineDiscountFactor(flowTime, pow(1.0 + getRate(), flowTime), CurveType::Discount());
		}
	}

	double CrossCurrencySwap::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return m_fixedLeg->getValue(*model);
	}

	void CrossCurrencySwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
		m_fixedLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd);
	}

	void CrossCurrencySwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		m_fixedLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd, curveType);
	}

	double CrossCurrencySwap::computeParRate(const BaseModelPtr& model)
	{
        double forPV = 0.0;

        if( m_singleCurrencyBasisSwap )
        {
            if( m_foreignLeg )
            {
                initializeForeignLegPricing(model);
                forPV = m_foreignLeg->getValue(*m_foreignModel);
            }
            return (forPV - m_floatingLeg->getValue(*model)) / m_fixedLeg->getValue(*model); 
        }

        if( m_foreignLeg )
        {
             initializeForeignLegPricing(model);
             forPV = fxRate(model) * (m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel))/m_fxSpot;
        }
        else if( m_resettableForeignLeg )
        {
             initializeForeignLegPricing(model);
             double fx = fxRate(model);
             forPV = fx * m_resettableForeignLeg->getValue(*m_foreignModel, *model, m_fxSpot);
        }

		if( domesticLegType() == IDeA::LegType::Floating )
		{
			return (m_firstDiscountFactor->getValue(*model) - m_lastDiscountFactor->getValue(*model) - m_floatingLeg->getValue(*model) + forPV) / m_fixedLeg->getValue(*model); 	
		}
		else
		{
			return (m_firstDiscountFactor->getValue(*model) - m_lastDiscountFactor->getValue(*model) + forPV) / m_fixedLeg->getValue(*model); 	
		}
	}

	void CrossCurrencySwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                         const BaseModel& model,
							             const double multiplier, 
							             IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{	
		
		if( domesticLegType() == IDeA::LegType::Floating )
		{	
			m_floatingLeg->fillRepFlows(assetDomain, model, multiplier, fundingRepFlows);
		}
		
		m_fixedLeg->fillRepFlows(assetDomain, model, multiplier * getRate(), fundingRepFlows);
		if( m_singleCurrencyBasisSwap )
        {
            if( m_foreignLeg )
            {
                initializeForeignLegPricing(model);
				AssetDomainConstPtr ad( new IRAssetDomain( m_foreignCcy, m_foreignMarket ) );
				m_foreignLeg->fillRepFlows(ad, *m_foreignModel, - multiplier, fundingRepFlows);
            }
            return;
        }
		fundingRepFlows.addRepFlow(IDeA::Funding::Key(model.primaryAssetDomainFunding(),m_lastDiscountFactor->getArguments().getPayDate()), multiplier);
		fundingRepFlows.addRepFlow(IDeA::Funding::Key(model.primaryAssetDomainFunding(),m_firstDiscountFactor->getArguments().getPayDate()), -multiplier);

		if( m_foreignLeg )
        {
             initializeForeignLegPricing(model);
			 AssetDomainConstPtr ad( new IRAssetDomain( m_foreignCcy, m_foreignMarket ) );
			 m_foreignLeg->fillRepFlows(ad, *m_foreignModel, - multiplier * fxRate(model)/m_fxSpot, fundingRepFlows);
			 fundingRepFlows.addRepFlow(IDeA::Funding::Key(ad,m_lastForeignDiscountFactor->getArguments().getPayDate()), -multiplier * fxRate(model)/m_fxSpot);
			 fundingRepFlows.addRepFlow(IDeA::Funding::Key(ad,m_firstForeignDiscountFactor->getArguments().getPayDate()), multiplier * fxRate(model)/m_fxSpot);
			 fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomain,m_fxSpotDomDiscountFactor->getArguments().getPayDate()), -multiplier * (m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel))/m_fxSpotForDiscountFactor->getValue(*m_foreignModel));
			 double forPV = fxRate(model) * (m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel))/m_fxSpot;
			 fundingRepFlows.addRepFlow(IDeA::Funding::Key(ad,m_fxSpotForDiscountFactor->getArguments().getPayDate()), multiplier * forPV/m_fxSpotForDiscountFactor->getValue(*m_foreignModel));
        }
		else if( m_resettableForeignLeg )
        {
            initializeForeignLegPricing(model);
            double fx = fxRate(model);
			AssetDomainConstPtr ad( new IRAssetDomain( m_foreignCcy, m_foreignMarket ) );
			double forPV = m_resettableForeignLeg->getValue(*m_foreignModel, model, m_fxSpot);
			
			fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomain,m_fxSpotDomDiscountFactor->getArguments().getPayDate()),- multiplier * fx/m_fxSpotDomDiscountFactor->getValue(model) * forPV);
			fundingRepFlows.addRepFlow(IDeA::Funding::Key(ad,m_fxSpotForDiscountFactor->getArguments().getPayDate()), multiplier * forPV/m_fxSpotForDiscountFactor->getValue(*m_foreignModel));
            m_resettableForeignLeg->fillRepFlows(ad,*m_foreignModel, assetDomain, model, m_fxSpot, - multiplier * fx, fundingRepFlows);

        }
	}

	void CrossCurrencySwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                         const BaseModel& model,
							             const double multiplier, 
							             IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			m_floatingLeg->fillRepFlows(assetDomain, model, multiplier, indexRepFlows);
		}
		if( m_singleCurrencyBasisSwap )
        {
            if( m_foreignLeg )
            {
                initializeForeignLegPricing(model);
				AssetDomainConstPtr ad( new IRAssetDomain( m_foreignCcy, m_foreignMarket ) );
				m_foreignLeg->fillRepFlows(ad, *m_foreignModel, - multiplier, indexRepFlows);
            }
            return;
        }
		if( m_foreignLeg )
        {
             initializeForeignLegPricing(model);
			 AssetDomainConstPtr ad( new IRAssetDomain( m_foreignCcy, m_foreignMarket ) );
			 m_foreignLeg->fillRepFlows(ad, *m_foreignModel, - multiplier * fxRate(model)/m_fxSpot, indexRepFlows);
        }
		else if( m_resettableForeignLeg )
        {
            initializeForeignLegPricing(model);
            double fx = fxRate(model);
			AssetDomainConstPtr ad( new IRAssetDomain( m_foreignCcy, m_foreignMarket ) );
			
            m_resettableForeignLeg->fillRepFlows(ad,*m_foreignModel, assetDomain, model, m_fxSpot, - multiplier * fx, indexRepFlows);
        }
	}

	double CrossCurrencySwap::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<CrossCurrencySwap>(), IDeA_KEY(CCYBASIS, TENOR), IDeA_KEY(CCYBASIS, SPREAD));
	}

    ostream& CrossCurrencySwap::print(ostream& out) const
    {
        out << "X" << getDescription().string();
        return out;
    }

    /**
        @brief Clone this cross currency swap.

        Uses a lookup to maintain directed graph relationships.

        @param lookup A lookup of previuosly created clones.
        @return       The clone of this instance.
    */
    ICloneLookupPtr CrossCurrencySwap::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new CrossCurrencySwap(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    CrossCurrencySwap::CrossCurrencySwap(CrossCurrencySwap const& original, CloneLookup& lookup) : 
        CalibrationInstrument(original),
        m_domesticLegTenor(original.m_domesticLegTenor),
        m_details(original.m_details),
        m_floatingLeg(lookup.get(original.m_floatingLeg)),
        m_fixedLeg(lookup.get(original.m_fixedLeg)),
        m_firstDiscountFactor(lookup.get(original.m_firstDiscountFactor)),
        m_lastDiscountFactor(lookup.get(original.m_lastDiscountFactor)),
        m_isInitialized(false),
        m_foreignLeg(lookup.get(original.m_foreignLeg)),
        m_resettableForeignLeg(lookup.get(original.m_resettableForeignLeg)),
        m_foreignCcy(original.m_foreignCcy),
        m_foreignMarket(original.m_foreignMarket),
        m_fxSpotDomDiscountFactor(lookup.get(original.m_fxSpotDomDiscountFactor)),
        m_fxSpotForDiscountFactor(lookup.get(original.m_fxSpotForDiscountFactor)),
        m_firstForeignDiscountFactor(lookup.get(original.m_firstForeignDiscountFactor)),
        m_lastForeignDiscountFactor(lookup.get(original.m_lastForeignDiscountFactor)),
        m_singleCurrencyBasisSwap(original.m_singleCurrencyBasisSwap)
    {
    }

    void CrossCurrencySwap::fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const
	{
		cashFlowsTable.set<LT::date>("First Payment Date", 0, m_firstDiscountFactor->getArguments().getPayDate());
		cashFlowsTable.set<LT::date>("Last Payment Date", 0, m_lastDiscountFactor->getArguments().getPayDate());
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			cashFlowsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_floatingLeg->getCashFlows());
			//cashFlowsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_fixedLeg->getCashFlows());
		}
		else
		{
			cashFlowsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_fixedLeg->getCashFlows());	
		}

        if( m_foreignLeg )
        {
            cashFlowsTable.set<LT::date>("First Foreign Payment Date", 0, m_firstForeignDiscountFactor->getArguments().getPayDate());
		    cashFlowsTable.set<LT::date>("Last Foreign Payment Date", 0, m_lastForeignDiscountFactor->getArguments().getPayDate());
            cashFlowsTable.set<LTQuant::GenericDataPtr>("Foreign Leg", 0, m_foreignLeg->getCashFlows());
        }
        else if( m_resettableForeignLeg)
        {
            cashFlowsTable.set<LTQuant::GenericDataPtr>("Resettable Foreign Leg", 0, m_resettableForeignLeg->getCashFlows());
        }
	}

	void CrossCurrencySwap::fillCashFlowPVsTable(const BaseModel& model,
												 LTQuant::GenericData& cashFlowPVsTable) const
	{
		cashFlowPVsTable.set<LT::date>("First Payment Date", 0, m_firstDiscountFactor->getArguments().getPayDate());
		cashFlowPVsTable.set<double>("First Payment PV", 0, m_firstDiscountFactor->getValue(model));

		cashFlowPVsTable.set<LT::date>("Last Payment Date", 0, m_lastDiscountFactor->getArguments().getPayDate());
		cashFlowPVsTable.set<double>("Last Payment PV", 0, m_lastDiscountFactor->getValue(model));
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_floatingLeg->computeCashFlowPVs(model));
			// cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Spreads", 0, m_fixedLeg->computeCashFlowPVs(model));
		}
		else
		{
			cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_fixedLeg->computeCashFlowPVs(model));
		}
         
        if( m_foreignLeg )
        {
           initializeForeignLegPricing(model);
           cashFlowPVsTable.set<LT::date>("First Foreign Payment Date", 0, m_firstForeignDiscountFactor->getArguments().getPayDate());
		   cashFlowPVsTable.set<double>("First Foreign Payment PV", 0, m_firstForeignDiscountFactor->getValue(*m_foreignModel));

		   cashFlowPVsTable.set<LT::date>("Last Foreign Payment Date", 0, m_lastForeignDiscountFactor->getArguments().getPayDate());
	 	   cashFlowPVsTable.set<double>("Last Foreign Payment PV", 0, m_lastForeignDiscountFactor->getValue(*m_foreignModel));
           cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Foreign Leg", 0, m_foreignLeg->computeCashFlowPVs(*m_foreignModel));
        }
        else if( m_resettableForeignLeg)
        {
            initializeForeignLegPricing(model);
            cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Resettable Foreign Leg", 0, m_resettableForeignLeg->computeCashFlowPVs(*m_foreignModel, model, m_fxSpot));
        }

	}

     void CrossCurrencySwap::finishCalibration(const BaseModelPtr model)
     {
        CalibrationInstrument::finishCalibration(model);
        m_floatingLeg->cleanupCashFlows();
        m_fixedLeg->cleanupCashFlows();
        m_isInitialized=false;
        //m_firstDiscountFactor.reset();
        //m_lastDiscountFactor.reset();
        if(m_foreignLeg)
        {
            m_foreignLeg->cleanupCashFlows();
           // m_firstForeignDiscountFactor.reset();
           // m_lastForeignDiscountFactor.reset();
            
           // m_fxSpotDomDiscountFactor.reset();
           // m_fxSpotForDiscountFactor.reset();
        }
        if(m_resettableForeignLeg)
        {
             m_resettableForeignLeg->cleanupCashFlows();
        }
     }

     void CrossCurrencySwap::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
     {
         const CrossCurrencySwapPtr ourTypeSrc=std::tr1::static_pointer_cast<CrossCurrencySwap>(src);
         if(!ourTypeSrc)
         {
             LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
         }

         m_firstDiscountFactor=ourTypeSrc->m_firstDiscountFactor;
         m_lastDiscountFactor=ourTypeSrc->m_lastDiscountFactor;

         m_isInitialized=false;

         CalibrationInstrument::reloadInternalState(src);
     }

     void CrossCurrencySwap::initializeForeignLegPricing(const BaseModelPtr model) const
        {
            initializeForeignLegPricing(*(model.get()));
        }
         
        void CrossCurrencySwap::initializeForeignLegPricing(const BaseModel& model) const
        {
            if( !m_isInitialized )
            {
                if( !model.getDependentMarketData() )
                {
                    LTQC_THROW( IDeA::ModelException, "Cross currency swap: unable to find any dependencies");
                }
                for (size_t i = 1; i < model.getDependentMarketData()->table->rowsGet(); ++i) 
                {
                    AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                    LT::Str asset = IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
                    if (adType == IDeA::AssetDomainType::IR )
                    {
                        if( asset.compareCaseless(m_details.m_forCurrency) == 0 )
                        {
                            m_foreignMarket = IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
                            m_foreignCcy = asset;
                            break;
                        }
                    }
                }
                
                if(model.hasDependentModel(IRAssetDomain(m_foreignCcy, m_foreignMarket)))
                {
                    m_foreignModel = model.getDependentModel(IRAssetDomain(m_foreignCcy, m_foreignMarket));
                }
                else
                {
                    LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model for " << m_details.m_forCurrency.data());
                }


                if( !m_singleCurrencyBasisSwap )
                {
                    m_fxSpot = model.getDependentFXRate(FXSpotAssetDomain(m_foreignCcy, m_details.m_currency));
              
                    LT::date valueDate = model.getValueDate();
                    std::string fxPair = m_foreignCcy.string() + m_details.m_currency.string();
                    LT::date fxSpot = LTQuant::getFXSpotDate(valueDate, fxPair);
                    m_fxSpotDomDiscountFactor = DiscountFactor::create(DiscountFactorArguments(valueDate, fxSpot,m_details.m_currency));
                    m_fxSpotForDiscountFactor = DiscountFactor::create(DiscountFactorArguments(valueDate, fxSpot,m_foreignCcy));
                }
                else
                {
                    m_fxSpot = 1.0;
                    LT::date valueDate = model.getValueDate();
                   
                    m_fxSpotDomDiscountFactor = DiscountFactor::create(DiscountFactorArguments(valueDate, valueDate,m_details.m_currency));
                    m_fxSpotForDiscountFactor = DiscountFactor::create(DiscountFactorArguments(valueDate, valueDate,m_foreignCcy));
                }
                m_isInitialized = true;
            }
        }
}   // FlexYCF