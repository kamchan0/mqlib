#include "stdafx.h"
#include "OISBasisSwap.h"
#include "BaseModel.h"
#include "Data/GenericData.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"
#include "AllComponentsAndCaches.h"
#include "GlobalComponentCache.h"
#include "TenorUtils.h"
#include "DataExtraction.h"
#include "FlexYcfUtils.h"
#include "FlexYCFCloneLookup.h"
#include "src/Enums/CompoundingType.h"

// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "MarketConvention.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<OISBasisSwap>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, OISBASIS);
	}

    using namespace LTQuant;
    
	OISBasisSwap::OISBasisSwap(const string& description,
									   const LT::date fixingDate,
									   const LT::date startDate,
									   const LT::date endDate,
                                       const LT::date lastDate,
									   const double swapRate,
									   const IDeA::OISBasisSwapMktConvention& tradeDetails,
									   const LT::date valueDate):					 
		CalibrationInstrument(swapRate, getKeyName<OISBasisSwap>(), description, fixingDate, startDate, lastDate),
        m_fixedSwapLeg(FixedLeg::create(FixedLeg::Arguments(valueDate,
													    fixingDate,
														startDate,
														endDate,
														tradeDetails.m_spreadFrequency.asTenorString().string(),
														tradeDetails.m_spreadAccrualBasis.asString().data(),
														tradeDetails.m_spreadAccrualCalendar.string(),
                                                        tradeDetails.m_oisMktConvention.m_currency,
                                                        tradeDetails.m_oisMktConvention.m_index,
														LTQC::RollConvMethod::ModifiedFollowing,
														tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rollRuleConvention,
														string("0B"),
														tradeDetails.m_oisMktConvention.m_payCalendar,
                                                        "Next",
                                                        tradeDetails.m_oisMktConvention.m_stubType
														))),
        m_floatingSwapLeg(FloatingLeg::create(FloatingLeg::Arguments(valueDate,fixingDate,startDate,
                                                                 endDate,
                                                                 tradeDetails.m_floatFrequency.asTenorString().string(),
																 tradeDetails.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_floatingDepRateMktConvention,
																 EndDateCalculationType::NotAdjusted,
																 LTQC::Tenor("0B"),
																 LTQC::DayCount::create(tradeDetails.m_floatAccrualBasis),
                                                                 tradeDetails.m_oisMktConvention.m_stubType))),
		m_fixedOISLeg(FixedLeg::create(FixedLeg::Arguments(valueDate,
													    fixingDate,
														startDate,
														endDate,
														tradeDetails.m_spreadFrequency.asTenorString().string(),
														tradeDetails.m_spreadAccrualBasis.asString().data(),
														tradeDetails.m_spreadAccrualCalendar.string(),
                                                        tradeDetails.m_oisMktConvention.m_currency,
                                                        tradeDetails.m_oisMktConvention.m_index,
														LTQC::RollConvMethod::ModifiedFollowing,
														tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rollRuleConvention,
														tradeDetails.m_oisMktConvention.m_payDelay.asTenorString().string(),
														tradeDetails.m_oisMktConvention.m_payCalendar,
                                                        "Next",
                                                        tradeDetails.m_oisMktConvention.m_stubType
														))),
        m_oisLeg(FloatingLeg::create(FloatingLeg::Arguments(valueDate,fixingDate,startDate,endDate,
                                                                 tradeDetails.m_floatFrequency.asTenorString().string(),
																 tradeDetails.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_oisMktConvention.m_depRateMktConvention,
																 EndDateCalculationType::NotAdjusted,
																 tradeDetails.m_oisMktConvention.m_payDelay,
																 LTQC::DayCount::create(tradeDetails.m_floatAccrualBasis),
                                                                 tradeDetails.m_oisMktConvention.m_stubType))),
        
        m_spreadLegTenor(tenorDescToYears(tradeDetails.m_spreadFrequency.asTenorString().string())),
        m_floatingLegTenor(tenorDescToYears(tradeDetails.m_floatFrequency.asTenorString().string())),
		m_details(tradeDetails)
    {
    }

    OISBasisSwap::OISBasisSwap(const string& description,
									   const LT::date fixingDate,
									   const LT::date startDate,
									   const LT::date endDate,
                                       const LT::date lastDate,
									   const double swapRate,
									   const IDeA::OISBasisSwapMktConvention& tradeDetails,
									   const LT::date valueDate,
                                       GlobalComponentCache& globalComponentCache):
		CalibrationInstrument(swapRate, getKeyName<OISBasisSwap>(), description, fixingDate, startDate, lastDate),
        m_fixedSwapLeg(FixedLeg::create(FixedLeg::Arguments(valueDate,
													    fixingDate,
														startDate,
														endDate,
														tradeDetails.m_spreadFrequency.asTenorString().string(),
														tradeDetails.m_spreadAccrualBasis.asString().data(),
														tradeDetails.m_spreadAccrualCalendar.string(),
                                                        tradeDetails.m_oisMktConvention.m_currency,
                                                        tradeDetails.m_oisMktConvention.m_index,
														LTQC::RollConvMethod::ModifiedFollowing,
														LTQC::RollRuleMethod::BusinessEOM,
														string("0B"),
														tradeDetails.m_oisMktConvention.m_payCalendar,
                                                        "Next",
                                                        tradeDetails.m_oisMktConvention.m_stubType
														))),
        m_floatingSwapLeg(globalComponentCache.get( FloatingLeg::Arguments(valueDate,
																 fixingDate,
                                                                 startDate,
                                                                 endDate,
                                                                 tradeDetails.m_floatFrequency.asTenorString().string(),
 																 tradeDetails.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_floatingDepRateMktConvention,
																 EndDateCalculationType::NotAdjusted,
																 LTQC::Tenor("0B"),
																 LTQC::DayCount::create(tradeDetails.m_floatAccrualBasis),
                                                                 tradeDetails.m_oisMktConvention.m_stubType))),
		m_fixedOISLeg(globalComponentCache.get( FixedLeg::Arguments(valueDate,
														fixingDate,
                                                        startDate,
                                                        endDate,
                                                        tradeDetails.m_spreadFrequency.asTenorString().string(),
                                                        tradeDetails.m_spreadAccrualBasis.asString().data(),
														tradeDetails.m_spreadAccrualCalendar.string(),
                                                        tradeDetails.m_oisMktConvention.m_currency,
                                                        tradeDetails.m_oisMktConvention.m_index,
														LTQC::RollConvMethod::ModifiedFollowing,
														LTQC::RollRuleMethod::BusinessEOM,
														tradeDetails.m_oisMktConvention.m_payDelay.asTenorString().string(),
														tradeDetails.m_oisMktConvention.m_payCalendar,
                                                        "Next",
                                                        tradeDetails.m_oisMktConvention.m_stubType
														))),
        m_oisLeg(globalComponentCache.get( FloatingLeg::Arguments(valueDate,
																 fixingDate,
                                                                 startDate,
                                                                 endDate,
                                                                 tradeDetails.m_floatFrequency.asTenorString().string(),
 																 tradeDetails.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_oisMktConvention.m_depRateMktConvention,
																 EndDateCalculationType::NotAdjusted,
																 tradeDetails.m_oisMktConvention.m_payDelay,
																 LTQC::DayCount::create(tradeDetails.m_floatAccrualBasis),
                                                                 tradeDetails.m_oisMktConvention.m_stubType))),
        
        m_spreadLegTenor(tenorDescToYears(tradeDetails.m_spreadFrequency.asTenorString().string())),
        m_floatingLegTenor(tenorDescToYears(tradeDetails.m_floatFrequency.asTenorString().string())),
		m_details(tradeDetails)
    {
    }

	CalibrationInstrumentPtr OISBasisSwap::create(	const LTQuant::GenericData& instrumentTable, 
													const LT::Ptr<LT::date>& buildDate,
													const LTQuant::GenericData& curveParametersTable,
													const LTQuant::GenericDataPtr&)
	{
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(curveParametersTable));				
		IDeA::OISBasisSwapMktConvention& tradeDetails = conventions->m_oisBasisSwap;

    	 string fixedTenor, floatingTenor, fixedBasisName, indexName, spotDaysStr, payDelayStr, payCalendarStr, accrualCalendarStr, ONBasis, floatingBasis, stubTypeStr, swapRollRule, rollRuleStr, compoundingTypeStr;  
		 double rateCutOff;
      

		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ACCRUALCALENDAR), accrualCalendarStr, tradeDetails.m_spreadAccrualCalendar.string());
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPREADTENOR),  fixedTenor, tradeDetails.m_spreadFrequency.asTenorString().string());
        IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FLOATTENOR),  floatingTenor, tradeDetails.m_floatFrequency.asTenorString().string()); 
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPREADACCRUALBASIS),  fixedBasisName, tradeDetails.m_spreadAccrualBasis.asString().data());
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FLOATBASIS),  floatingBasis, tradeDetails.m_floatAccrualBasis.asString().data());
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, RATESBASIS),	   ONBasis, tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_dcm.asString().data());
        IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, PAYDELAY),    payDelayStr, tradeDetails.m_oisMktConvention.m_payDelay.asTenorString().string()); 
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, PAYCALENDAR), payCalendarStr, tradeDetails.m_oisMktConvention.m_payCalendar.string());
        IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STUBTYPE), stubTypeStr, string());
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, COMPOUNDINGTYPE), compoundingTypeStr, string());
		IDeA::permissive_extract<double>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, RATECUTOFF),  rateCutOff, 1.0);
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, INDEX),       indexName, tradeDetails.m_oisMktConvention.m_index.string());
        IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPOTDAYS),    spotDaysStr, tradeDetails.m_oisMktConvention.m_spotDays.asTenorString().string());
		if( IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ROLLRULEMETHOD), swapRollRule, rollRuleStr) )
		{
			tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rollRuleConvention = LTQC::RollRuleMethod(swapRollRule);
			tradeDetails.m_floatingDepRateMktConvention.m_rollRuleConvention = LTQC::RollRuleMethod(swapRollRule);
		}

        tradeDetails.m_floatingDepRateMktConvention.m_rateTenor = LTQC::Tenor(floatingTenor);

        tradeDetails.m_spreadAccrualCalendar =  LT::Str(accrualCalendarStr); 
        tradeDetails.m_floatAccrualCalendar =  LT::Str(accrualCalendarStr); 
		tradeDetails.m_spreadFrequency = LTQC::Tenor(fixedTenor);
		tradeDetails.m_spreadAccrualBasis = LTQC::DayCountMethod(fixedBasisName);
		tradeDetails.m_floatAccrualBasis = LTQC::DayCountMethod(floatingBasis);
		tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_dcm = LTQC::DayCountMethod(ONBasis);
		tradeDetails.m_floatFrequency = LTQC::Tenor(floatingTenor);
		tradeDetails.m_oisMktConvention.m_spotDays = LTQC::Tenor(spotDaysStr);
		tradeDetails.m_oisMktConvention.m_payDelay = LTQC::Tenor(payDelayStr);
		tradeDetails.m_oisMktConvention.m_payCalendar = LT::Str(payCalendarStr);
        if( !stubTypeStr.empty() )
        {
            tradeDetails.m_oisMktConvention.m_stubType = LTQC::StubType(stubTypeStr);
        }
        tradeDetails.m_oisMktConvention.m_index = LT::Str(indexName);
		tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rateTenor = LTQC::Tenor("1D");
		tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_depositRateType = DepositRateType::ON;
		if( !compoundingTypeStr.empty() )
        {
			if( IDeA::CompoundingType(LT::Str(compoundingTypeStr)) == IDeA::CompoundingType::WeightedAverage)
			{
				tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_depositRateType = DepositRateType::OnArithmetic;
				tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rateCutOff = static_cast<size_t>(rateCutOff);
			}
			else
			{
				if( IDeA::CompoundingType(LT::Str(compoundingTypeStr)) != IDeA::CompoundingType::Compounding)
				{
					LTQC_THROW(IDeA::MarketException, "OISBasisSwap: Compounding type not implemented: " << compoundingTypeStr);
				}
			}
        }

        std::string maturity;
		LT::date fixingDate, actualBuildDate, startDate, endDate, lastDate;
        bool foundStartDate = IDeA::permissive_extract<LT::date>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE), startDate, LT::date() );
		if ( foundStartDate  )
        {
            setMaturityAndDates(instrumentTable, buildDate, tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_fixingCalendar, tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_accrualValueCalendar, tradeDetails.m_oisMktConvention.m_spotDays, startDate, maturity, actualBuildDate, fixingDate, endDate);
        }
        else
        {
            setMaturityAndDates(instrumentTable, buildDate, tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_fixingCalendar, tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_accrualValueCalendar, tradeDetails.m_oisMktConvention.m_spotDays, maturity, actualBuildDate, fixingDate, startDate, endDate);
        }
        // endDate has to be adjusted using payDelay
		lastDate = addDatePeriod(endDate, payDelayStr, ModuleDate::CalendarFactory::create(payCalendarStr));
		return CalibrationInstrumentPtr(new OISBasisSwap(maturity, fixingDate, startDate, endDate, lastDate, 0.0, tradeDetails, actualBuildDate));
	}

    void OISBasisSwap::createInstruments(CalibrationInstruments& instruments,
                                             LTQuant::GenericDataPtr instrumentTable,
                                             LTQuant::GenericDataPtr masterTable,
                                             GlobalComponentCache& globalComponentCache,
                                             const LTQuant::PriceSupplierPtr)
    {
		const size_t nbVanillaSwaps(IDeA::numberOfRecords(*instrumentTable));

        // just finish if we have empty table or just headings
        if(nbVanillaSwaps == 0)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		const LT::date fixingDate(valueDate);

        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		if (!parametersTable)
			LTQC_THROW(IDeA::MarketException, "Null parameters table");

		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));				
		IDeA::OISBasisSwapMktConvention& tradeDetails = conventions->m_oisBasisSwap;
		
		//	Model
		// Swap Fixing
        for(size_t cnt(0); cnt < nbVanillaSwaps; ++cnt)
        {
			const std::string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(OISBASIS, TENOR), cnt));
            
            if(!description.empty())
            {
                const double swapRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(OISBASIS, RATE), cnt));

                

                string fixedTenor, floatingTenor, fixedBasisName, indexName, spotDaysStr, payDelayStr, payCalendarStr, accrualCalendarStr, ONBasis, floatingBasis ,stubTypeStr, swapRollRule, rollRuleStr, compoundingTypeStr;
				double rateCutOff;

				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, ACCRUALCALENDAR),  cnt, accrualCalendarStr, tradeDetails.m_spreadAccrualCalendar.string());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, SPREADTENOR),  cnt, fixedTenor, tradeDetails.m_spreadFrequency.asTenorString().string());
                IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, FLOATTENOR),  cnt, floatingTenor, tradeDetails.m_floatFrequency.asTenorString().string()); 
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, SPREADBASIS),  cnt, fixedBasisName, tradeDetails.m_spreadAccrualBasis.asString().data());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, FLOATBASIS),  cnt, floatingBasis, tradeDetails.m_floatAccrualBasis.asString().data());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, RATESBASIS),	  cnt, ONBasis, tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_dcm.asString().data());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, SPOTDAYS),    cnt, spotDaysStr, tradeDetails.m_oisMktConvention.m_spotDays.asTenorString().string());
                IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, PAYDELAY),    cnt, payDelayStr, tradeDetails.m_oisMktConvention.m_payDelay.asTenorString().string()); 
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, PAYCALENDAR), cnt, payCalendarStr, tradeDetails.m_oisMktConvention.m_payCalendar.string());
                IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, STUBTYPE), cnt,   stubTypeStr, string());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, COMPOUNDINGTYPE), cnt,   compoundingTypeStr, string());
				IDeA::permissive_extract<double>(instrumentTable, IDeA_KEY(OISBASIS, RATECUTOFF), cnt,  rateCutOff, 1.0);
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OISBASIS, INDEX),       cnt, indexName, tradeDetails.m_oisMktConvention.m_index.string());
				if( IDeA::permissive_extract<std::string>(instrumentTable, IDeA_KEY(OISBASIS, ROLLRULEMETHOD), cnt, swapRollRule, rollRuleStr) )
				{
					tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rollRuleConvention = LTQC::RollRuleMethod(swapRollRule);
					tradeDetails.m_floatingDepRateMktConvention.m_rollRuleConvention = LTQC::RollRuleMethod(swapRollRule);
				}
                tradeDetails.m_floatingDepRateMktConvention.m_rateTenor = LTQC::Tenor(floatingTenor);
				tradeDetails.m_spreadAccrualCalendar =  LT::Str(accrualCalendarStr); 
                tradeDetails.m_floatAccrualCalendar =  LT::Str(accrualCalendarStr); 
				tradeDetails.m_spreadFrequency = LTQC::Tenor(fixedTenor);
				tradeDetails.m_spreadAccrualBasis = LTQC::DayCountMethod(fixedBasisName);
				tradeDetails.m_floatAccrualBasis = LTQC::DayCountMethod(floatingBasis);
				tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_dcm = LTQC::DayCountMethod(ONBasis);
				tradeDetails.m_floatFrequency = LTQC::Tenor(floatingTenor);
				tradeDetails.m_oisMktConvention.m_spotDays = LTQC::Tenor(spotDaysStr);
				tradeDetails.m_oisMktConvention.m_payDelay = LTQC::Tenor(payDelayStr);
				tradeDetails.m_oisMktConvention.m_payCalendar = LT::Str(payCalendarStr);
                if( !stubTypeStr.empty() )
                {
                    tradeDetails.m_oisMktConvention.m_stubType = LTQC::StubType(stubTypeStr);
                }
                tradeDetails.m_oisMktConvention.m_index = LT::Str(indexName);
				tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rateTenor = LTQC::Tenor("1D");
				tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_depositRateType = DepositRateType::ON;
				if( !compoundingTypeStr.empty() )
				{
					if( IDeA::CompoundingType(LT::Str(compoundingTypeStr)) == IDeA::CompoundingType::WeightedAverage)
					{
						tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_depositRateType = DepositRateType::OnArithmetic;
						tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rateCutOff = static_cast<size_t>(rateCutOff);
					}
					else
					{
						if( IDeA::CompoundingType(LT::Str(compoundingTypeStr)) != IDeA::CompoundingType::Compounding)
						{
							LTQC_THROW(IDeA::MarketException, "OISBasisSwap: Compounding type not implemented: " << compoundingTypeStr);
						}
					}
				}

				LT::date startDate, endDate, lastDate;
                bool foundStartDate = IDeA::permissive_extract<LT::date>(instrumentTable, IDeA_KEY(OISBASIS, STARTDATE), cnt, startDate, LT::date() );
		        if ( foundStartDate )
                {
                    fillEndDate(instrumentTable, cnt, description, tradeDetails.m_spreadAccrualCalendar.data(), startDate, endDate);
                }
                else
                {
                    fillStartAndEndDates(instrumentTable, cnt, description, tradeDetails.m_oisMktConvention.m_fixingCalendar.data(), tradeDetails.m_spreadAccrualCalendar.data(), tradeDetails.m_oisMktConvention.m_spotDays, fixingDate, startDate, endDate);
                }
              
				
				// endDate has to be adjusted using payDelay
				lastDate = addDatePeriod(endDate, payDelayStr, ModuleDate::CalendarFactory::create(payCalendarStr));
				IDeA::permissive_extract<LT::date>(instrumentTable, IDeA_KEY(OISBASIS, LASTDATE), cnt, lastDate, lastDate);

				CalibrationInstrumentPtr instrument(new OISBasisSwap(description, fixingDate, startDate, endDate, lastDate, swapRate, tradeDetails, valueDate, globalComponentCache));
                instruments.add(instrument);
            }
        }
    }

    void OISBasisSwap::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);

        m_fixedSwapLeg->cleanupCashFlows();
        m_floatingSwapLeg->cleanupCashFlows();
        m_fixedOISLeg->cleanupCashFlows();
        m_oisLeg->cleanupCashFlows();
    }
    
     void OISBasisSwap::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr & src)
     {
         const OISBasisSwapPtr ourTypeSrc=std::tr1::static_pointer_cast<OISBasisSwap>(src);
         if(!ourTypeSrc)
         {
             LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
         }

         CalibrationInstrument::reloadInternalState(src);
     }

    void OISBasisSwap::updateInstruments(CalibrationInstruments& instrumentList, LTQuant::GenericDataPtr instrumentTable, size_t* instrumentIndex)
    {
		const size_t nbVanillaSwaps(IDeA::numberOfRecords(*instrumentTable));

        // just finish if we have empty table or just headings
        if(nbVanillaSwaps == 0)
        {
            return;
        }

        for(size_t i(0); i < nbVanillaSwaps; ++i)
        {
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(OISBASIS, TENOR), i));
            if(!description.empty())
            {
                const double swapRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(OISBASIS, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(swapRate);
                ++(*instrumentIndex);
            }
        }
    }
     
    double OISBasisSwap::getLastRelevantTime() const 
    {
        return ModuleDate::getYearsBetween(m_fixedOISLeg->getValueDate(), getEndDate()); 
    }
    
    const double OISBasisSwap::computeModelPrice(const BaseModelPtr model) const
    {
        double OISLegPv   = m_oisLeg->getValue(*model);
        double OISAnnuity = m_fixedOISLeg->getValue(*model);
        double parRateOIS =  OISLegPv/OISAnnuity;
     
        return m_floatingSwapLeg->getValue(*model) - m_fixedSwapLeg->getValue(*model) * ( parRateOIS + getRate() ); 
    }
        
    void OISBasisSwap::accumulateGradient(BaseModel const& model, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {
        double OISLegPv   = m_oisLeg->getValue(model);
        double OISAnnuity = m_fixedOISLeg->getValue(model);
        double parRateOIS =  OISLegPv/OISAnnuity;
        double fixedBasisSwapLegPv = m_fixedSwapLeg->getValue(model);

        m_fixedOISLeg->accumulateGradient(model, multiplier * OISLegPv * fixedBasisSwapLegPv/(OISAnnuity * OISAnnuity), gradientBegin, gradientEnd);
      
        m_oisLeg->accumulateGradient(model, - multiplier * fixedBasisSwapLegPv/OISAnnuity, gradientBegin, gradientEnd);

        m_floatingSwapLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd);
        m_fixedSwapLeg->accumulateGradient(model, - multiplier * ( parRateOIS + getRate() ), gradientBegin, gradientEnd);


    }

	void OISBasisSwap::accumulateGradient(BaseModel const& model,
											 double multiplier,
											 GradientIterator gradientBegin,
											 GradientIterator gradientEnd,
											 const CurveTypeConstPtr& curveType)
	{
        double OISLegPv   = m_oisLeg->getValue(model);
        double OISAnnuity = m_fixedOISLeg->getValue(model);
        double parRateOIS =  OISLegPv/OISAnnuity;
        double fixedBasisSwapLegPv = m_fixedSwapLeg->getValue(model);

        m_fixedOISLeg->accumulateGradient(model, multiplier * OISLegPv * fixedBasisSwapLegPv/(OISAnnuity * OISAnnuity), gradientBegin, gradientEnd,curveType);
      
        m_oisLeg->accumulateGradient(model, - multiplier * fixedBasisSwapLegPv/OISAnnuity, gradientBegin, gradientEnd,curveType);

        m_floatingSwapLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd,curveType);
        m_fixedSwapLeg->accumulateGradient(model, - multiplier * ( parRateOIS + getRate() ), gradientBegin, gradientEnd,curveType); 
	}
	
	void OISBasisSwap::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		USE(baseModel)
		USE(dfModel)
		USE(multiplier)
		USE(gradientBegin)
		USE(gradientEnd)
		USE(spread)
		LTQC_THROW(IDeA::SystemException, "accumulateGradientConstantDiscountFactor not implemented");
	}
    
	void OISBasisSwap::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		USE(baseModel)
		USE(dfModel)
		USE(multiplier)
		USE(gradientBegin)
		USE(gradientEnd)
		USE(spread)
		LTQC_THROW(IDeA::SystemException, "accumulateGradientConstantTenorDiscountFactor not implemented");
	}
    
	void OISBasisSwap::update()
    {
        m_fixedSwapLeg->update();
        m_floatingSwapLeg->update();
        m_fixedOISLeg->update();
        m_oisLeg->update();
    }

	double OISBasisSwap::getVariableInitialGuess(const double flowTime,
													 const BaseModel* const model) const
	{
		LT_LOG << "initial guess for OISBasis" << getDescription().string()<< ", " << CurveType::Discount()->getDescription() << " (flt tenor: " << m_floatingLegTenor << ")" << endl;
		return model->getVariableValueFromSpineDiscountFactor(flowTime, 1.0, CurveType::Discount());
	}

	double OISBasisSwap::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return - m_fixedSwapLeg->getValue(*model);
	}

	void OISBasisSwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
		m_fixedSwapLeg->accumulateGradient(model, - multiplier, gradientBegin, gradientEnd);
	}

	void OISBasisSwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		m_fixedSwapLeg->accumulateGradient(model, - multiplier, gradientBegin, gradientEnd, curveType);
	}

	double OISBasisSwap::computeParRate(const BaseModelPtr& model)
	{
		return m_floatingSwapLeg->getValue(*model)/m_fixedSwapLeg->getValue(*model) - m_oisLeg->getValue(*model)/ m_fixedOISLeg->getValue(*model);
	}
	
	void OISBasisSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                            const BaseModel& model,
										    const double multiplier, 
										    IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{ 
        double OISLegPv   = m_oisLeg->getValue(model);
        double OISAnnuity = m_fixedOISLeg->getValue(model);
        double parRateOIS =  OISLegPv/OISAnnuity;
		
		m_floatingSwapLeg->fillRepFlows(assetDomain, model, multiplier, fundingRepFlows);
        m_fixedSwapLeg->fillRepFlows(assetDomain, model, - multiplier * (parRateOIS + getRate()), fundingRepFlows);
        
		m_fixedOISLeg->fillRepFlows(assetDomain, model, multiplier * parRateOIS, fundingRepFlows);
		m_oisLeg->fillRepFlows(assetDomain, model, - multiplier, fundingRepFlows);
	}

	void OISBasisSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                            const BaseModel& model,
										    const double multiplier, 
										    IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
    {
		
		m_floatingSwapLeg->fillRepFlows(assetDomain, model, multiplier, indexRepFlows);
		m_oisLeg->fillRepFlows(assetDomain, model, - multiplier, indexRepFlows);
	}

	double OISBasisSwap::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<OISBasisSwap>(), IDeA_KEY(OISBASIS, TENOR), IDeA_KEY(OISBASIS, RATE));
	}

    ostream& OISBasisSwap::print(ostream& out) const
    {
        out << "OISBasis" << getDescription().string();
        return out;
    }

   
    ICloneLookupPtr OISBasisSwap::cloneWithLookup(CloneLookup& lookup) const
    {
        return CalibrationInstrumentPtr(new OISBasisSwap(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    OISBasisSwap::OISBasisSwap(OISBasisSwap const& original, CloneLookup& lookup) : 
        CalibrationInstrument(original),
        m_details(original.m_details),
        m_spreadLegTenor(original.m_spreadLegTenor),
        m_floatingLegTenor(original.m_floatingLegTenor),
        m_fixedSwapLeg(lookup.get(original.m_fixedSwapLeg)),
        m_floatingSwapLeg(lookup.get(original.m_floatingSwapLeg)),
        m_fixedOISLeg(lookup.get(original.m_fixedOISLeg)),
        m_oisLeg(lookup.get(original.m_oisLeg))
    {
    }

	void OISBasisSwap::fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const
	{
		cashFlowsTable.set<LTQuant::GenericDataPtr>("Fixed Leg Swap", 0, m_fixedSwapLeg->getCashFlows());
		cashFlowsTable.set<LTQuant::GenericDataPtr>("Floating Leg Swap", 0, m_floatingSwapLeg->getCashFlows());
        cashFlowsTable.set<LTQuant::GenericDataPtr>("Fixed Leg OIS", 0, m_fixedOISLeg->getCashFlows());
        cashFlowsTable.set<LTQuant::GenericDataPtr>("Floating Leg OIS", 0, m_oisLeg->getCashFlows());
	}

	void OISBasisSwap::fillCashFlowPVsTable(const BaseModel& model, LTQuant::GenericData& cashFlowPVsTable) const
	{

		cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Fixed Leg Swap", 0, m_fixedSwapLeg->computeCashFlowPVs(model));
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Floating Leg Swap", 0, m_floatingSwapLeg->computeCashFlowPVs(model));
        cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Fixed Leg OIS", 0, m_fixedOISLeg->computeCashFlowPVs(model));
        cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Floating Leg OIS", 0, m_oisLeg->computeCashFlowPVs(model));
	}
}   // FlexYCF