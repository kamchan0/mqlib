#include "stdafx.h"
#include "OvernightIndexedSwap.h"
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
	const IDeA::DictionaryKey& getKey<OvernightIndexedSwap>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, OIS);
	}

    using namespace LTQuant;
    
	OvernightIndexedSwap::OvernightIndexedSwap(const string& description,
									   const LT::date fixingDate,
									   const LT::date startDate,
									   const LT::date endDate,
                                       const LT::date lastDate,
									   const double swapRate,
									   const IDeA::OISSwapMktConvention& tradeDetails,
									   const LT::date valueDate):					 
		CalibrationInstrument(swapRate, getKeyName<OvernightIndexedSwap>(), description, fixingDate, startDate, lastDate),
		m_fixedLeg(FixedLeg::create(FixedLeg::Arguments(valueDate,
													    fixingDate,
														startDate,
														endDate,
														tradeDetails.m_fixedFrequency.asTenorString().string(),
														tradeDetails.m_fixedAccrualBasis.asString().data(),
														tradeDetails.m_fixedAccrualCalendar.string(),
                                                        tradeDetails.m_oisMktConvention.m_currency,
                                                        tradeDetails.m_oisMktConvention.m_index,
														LTQC::RollConvMethod::ModifiedFollowing,
														LTQC::RollRuleMethod::BusinessEOM,
														tradeDetails.m_oisMktConvention.m_payDelay.asTenorString().string(),
														tradeDetails.m_oisMktConvention.m_payCalendar,
                                                        LTQC::RollConvMethod(LTQC::RollConvMethod::Following).asString(),
                                                        tradeDetails.m_oisMktConvention.m_stubType
														))),
		m_floatingLeg(FloatingLeg::create(FloatingLeg::Arguments(valueDate,fixingDate,startDate,endDate,
                                                                 tradeDetails.m_oisMktConvention.m_floatFrequency.asTenorString().string(),
																 tradeDetails.m_oisMktConvention.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_oisMktConvention.m_depRateMktConvention,
																 EndDateCalculationType::NotAdjusted,
																 tradeDetails.m_oisMktConvention.m_payDelay,
																 LTQC::DayCount::create(tradeDetails.m_oisMktConvention.m_floatAccrualBasis),
                                                                 tradeDetails.m_oisMktConvention.m_stubType))),
        m_fixedLegTenor(tenorDescToYears(tradeDetails.m_fixedFrequency.asTenorString().string())),
        m_floatingLegTenor(tenorDescToYears(tradeDetails.m_oisMktConvention.m_floatFrequency.asTenorString().string())),
		m_details(tradeDetails)
    {
    }

    OvernightIndexedSwap::OvernightIndexedSwap(const string& description,
									   const LT::date fixingDate,
									   const LT::date startDate,
									   const LT::date endDate,
                                       const LT::date lastDate,
									   const double swapRate,
									   const IDeA::OISSwapMktConvention& tradeDetails,
									   const LT::date valueDate,
                                       GlobalComponentCache& globalComponentCache):
		CalibrationInstrument(swapRate, getKeyName<OvernightIndexedSwap>(), description, fixingDate, startDate, lastDate),
		m_fixedLeg(globalComponentCache.get( FixedLeg::Arguments(valueDate,
														fixingDate,
                                                        startDate,
                                                        endDate,
                                                        tradeDetails.m_fixedFrequency.asTenorString().string(),
                                                        tradeDetails.m_fixedAccrualBasis.asString().data(),
														tradeDetails.m_fixedAccrualCalendar.string(),
                                                        tradeDetails.m_oisMktConvention.m_currency,
                                                        tradeDetails.m_oisMktConvention.m_index,
														LTQC::RollConvMethod::ModifiedFollowing,
														tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_rollRuleConvention,
														tradeDetails.m_oisMktConvention.m_payDelay.asTenorString().string(),
														tradeDetails.m_oisMktConvention.m_payCalendar,
                                                        LTQC::RollConvMethod(LTQC::RollConvMethod::Following).asString(),
                                                        tradeDetails.m_oisMktConvention.m_stubType
														))),

		m_floatingLeg(globalComponentCache.get( FloatingLeg::Arguments(valueDate,
																 fixingDate,
                                                                 startDate,
                                                                 endDate,
                                                                 tradeDetails.m_oisMktConvention.m_floatFrequency.asTenorString().string(),
 																 tradeDetails.m_oisMktConvention.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_oisMktConvention.m_depRateMktConvention,
																 EndDateCalculationType::NotAdjusted,
																 tradeDetails.m_oisMktConvention.m_payDelay,
																 LTQC::DayCount::create(tradeDetails.m_oisMktConvention.m_floatAccrualBasis),
                                                                 tradeDetails.m_oisMktConvention.m_stubType))),
        m_fixedLegTenor(tenorDescToYears(tradeDetails.m_fixedFrequency.asTenorString().string())),
        m_floatingLegTenor(tenorDescToYears(tradeDetails.m_oisMktConvention.m_floatFrequency.asTenorString().string())),
		m_details(tradeDetails)
    {
    }

	CalibrationInstrumentPtr OvernightIndexedSwap::create(const LTQuant::GenericData& instrumentTable, 
														  const LT::Ptr<LT::date>& buildDate,
														  const LTQuant::GenericData& curveParametersTable,
														  const LTQuant::GenericDataPtr&)
	{
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(curveParametersTable));				
		IDeA::OISSwapMktConvention& tradeDetails = conventions->m_ois;

    	 string fixedTenor, floatingTenor, fixedBasisName, indexName, spotDaysStr, payDelayStr, payCalendarStr, accrualCalendarStr, ONBasis, floatingBasis, stubTypeStr, swapRollRule, rollRuleStr, compoundingTypeStr;  
		 double rateCutOff;
      

		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ACCRUALCALENDAR), accrualCalendarStr, tradeDetails.m_fixedAccrualCalendar.string());
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FIXEDTENOR),  fixedTenor, tradeDetails.m_fixedFrequency.asTenorString().string());
        IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FLOATTENOR),  floatingTenor, tradeDetails.m_oisMktConvention.m_floatFrequency.asTenorString().string()); 
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FIXEDBASIS),  fixedBasisName, tradeDetails.m_fixedAccrualBasis.asString().data());
		IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FLOATBASIS),  floatingBasis, tradeDetails.m_oisMktConvention.m_floatAccrualBasis.asString().data());
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
		}

        tradeDetails.m_fixedAccrualCalendar =  LT::Str(accrualCalendarStr); 
        tradeDetails.m_oisMktConvention.m_floatAccrualCalendar =  LT::Str(accrualCalendarStr); 
		tradeDetails.m_fixedFrequency = LTQC::Tenor(fixedTenor);
		tradeDetails.m_fixedAccrualBasis = LTQC::DayCountMethod(fixedBasisName);
		tradeDetails.m_oisMktConvention.m_floatAccrualBasis = LTQC::DayCountMethod(floatingBasis);
		tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_dcm = LTQC::DayCountMethod(ONBasis);
		tradeDetails.m_oisMktConvention.m_floatFrequency = LTQC::Tenor(floatingTenor);
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
					LTQC_THROW(IDeA::MarketException, "OISSwap: Compounding type not implemented: " << compoundingTypeStr);
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
		return CalibrationInstrumentPtr(new OvernightIndexedSwap(maturity, fixingDate, startDate, endDate, lastDate, 0.0, tradeDetails, actualBuildDate));
	}

    void OvernightIndexedSwap::createInstruments(CalibrationInstruments& instruments,
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
		
		//	Model
		// Swap Fixing
        for(size_t cnt(0); cnt < nbVanillaSwaps; ++cnt)
        {
			std::string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(OIS, TENOR), cnt));
            
            if(!description.empty())
            {
                const double swapRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(OIS, RATE), cnt));
                IDeA::OISSwapMktConvention tradeDetails = conventions->m_ois;
                

                string fixedTenor, floatingTenor, fixedBasisName, indexName, spotDaysStr, payDelayStr, payCalendarStr, accrualCalendarStr, ONBasis, floatingBasis ,stubTypeStr,compoundingTypeStr;
				double rateCutOff;

				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, ACCRUALCALENDAR),  cnt, accrualCalendarStr, tradeDetails.m_fixedAccrualCalendar.string());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, FIXEDTENOR),  cnt, fixedTenor, tradeDetails.m_fixedFrequency.asTenorString().string());
                IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, FLOATTENOR),  cnt, floatingTenor, tradeDetails.m_oisMktConvention.m_floatFrequency.asTenorString().string()); 
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, FIXEDBASIS),  cnt, fixedBasisName, tradeDetails.m_fixedAccrualBasis.asString().data());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, FLOATBASIS),  cnt, floatingBasis, tradeDetails.m_oisMktConvention.m_floatAccrualBasis.asString().data());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, RATESBASIS),	  cnt, ONBasis, tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_dcm.asString().data());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, SPOTDAYS),    cnt, spotDaysStr, tradeDetails.m_oisMktConvention.m_spotDays.asTenorString().string());
                IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, PAYDELAY),    cnt, payDelayStr, tradeDetails.m_oisMktConvention.m_payDelay.asTenorString().string()); 
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, PAYCALENDAR), cnt, payCalendarStr, tradeDetails.m_oisMktConvention.m_payCalendar.string());
                IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, STUBTYPE), cnt,   stubTypeStr, string());
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, COMPOUNDINGTYPE), cnt,   compoundingTypeStr, string());
				IDeA::permissive_extract<double>(instrumentTable, IDeA_KEY(OIS, RATECUTOFF), cnt,  rateCutOff, 1.0);
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(OIS, INDEX),       cnt, indexName, tradeDetails.m_oisMktConvention.m_index.string());

				tradeDetails.m_fixedAccrualCalendar =  LT::Str(accrualCalendarStr); 
                tradeDetails.m_oisMktConvention.m_floatAccrualCalendar =  LT::Str(accrualCalendarStr); 
				tradeDetails.m_fixedFrequency = LTQC::Tenor(fixedTenor);
				tradeDetails.m_fixedAccrualBasis = LTQC::DayCountMethod(fixedBasisName);
				tradeDetails.m_oisMktConvention.m_floatAccrualBasis = LTQC::DayCountMethod(floatingBasis);
				tradeDetails.m_oisMktConvention.m_depRateMktConvention.m_dcm = LTQC::DayCountMethod(ONBasis);
				tradeDetails.m_oisMktConvention.m_floatFrequency = LTQC::Tenor(floatingTenor);
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
							LTQC_THROW(IDeA::MarketException, "OISSwap: Compounding type not implemented: " << compoundingTypeStr);
						}
					}
				}
				

                // special tenors
                if( description.compare("O/N") == 0 || description.compare("ON") == 0 )
                {
                    description = "1D";
                    tradeDetails.m_oisMktConvention.m_spotDays = LTQC::Tenor("0D");
                }
                if( description.compare("T/N") == 0 || description.compare("TN") == 0 )
                {
                    description = "1D";
                    tradeDetails.m_oisMktConvention.m_spotDays = LTQC::Tenor("1D");
                }

				LT::Str descStr(description);
				if( descStr.compareCaseless ("RUNDOWN") == 0 )
				{
					LT::date start, end;
					bool foundStart = IDeA::permissive_extract<LT::date>(instrumentTable, IDeA_KEY(OIS, STARTDATE), cnt, start, LT::date() );
					if(!foundStart)
					{   
						fillStartAndEndDates(instrumentTable, cnt, "1m", tradeDetails.m_oisMktConvention.m_fixingCalendar.data(), tradeDetails.m_fixedAccrualCalendar.data(), tradeDetails.m_oisMktConvention.m_spotDays, fixingDate, start, end);
					}
					bool foundEnd = IDeA::permissive_extract<LT::date>(instrumentTable, IDeA_KEY(OIS, STARTDATE), cnt + 1, end, LT::date() );
					if(!foundEnd)
					{
						LTQC_THROW(IDeA::MarketException, "OIS RunDown instrument needs start date");
					}
					size_t t = end.getAsLong() - start.getAsLong();
					if(t == 0)
						continue;
					Tenor runDownTenor(t, 0, 0, 0, 0);
					description = runDownTenor.asTenorString().data();
				}

				LT::date startDate, endDate, lastDate;
                bool foundStartDate = IDeA::permissive_extract<LT::date>(instrumentTable, IDeA_KEY(OIS, STARTDATE), cnt, startDate, LT::date() );
		        if ( foundStartDate )
                {
                    fillEndDate(instrumentTable, cnt, description, tradeDetails.m_fixedAccrualCalendar.data(), startDate, endDate);
                }
                else
                {
                    fillStartAndEndDates(instrumentTable, cnt, description, tradeDetails.m_oisMktConvention.m_fixingCalendar.data(), tradeDetails.m_fixedAccrualCalendar.data(), tradeDetails.m_oisMktConvention.m_spotDays, fixingDate, startDate, endDate);
                }
              
                
				// endDate has to be adjusted using payDelay
				lastDate = addDatePeriod(endDate, payDelayStr, ModuleDate::CalendarFactory::create(payCalendarStr));
				
				CalibrationInstrumentPtr instrument(new OvernightIndexedSwap(description, fixingDate, startDate, endDate, lastDate, swapRate, tradeDetails, valueDate, globalComponentCache));
                instruments.add(instrument);
            }
        }
    }

    void OvernightIndexedSwap::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_floatingLeg->cleanupCashFlows();
        m_fixedLeg->cleanupCashFlows();
    }

    void OvernightIndexedSwap::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
        const OvernightIndexedSwapPtr ourTypeSrc=std::tr1::static_pointer_cast<OvernightIndexedSwap>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        CalibrationInstrument::reloadInternalState(src);
    }
    void OvernightIndexedSwap::updateInstruments(CalibrationInstruments& instrumentList, LTQuant::GenericDataPtr instrumentTable, size_t* instrumentIndex)
    {
		const size_t nbVanillaSwaps(IDeA::numberOfRecords(*instrumentTable));

        // just finish if we have empty table or just headings
        if(nbVanillaSwaps == 0)
        {
            return;
        }

        for(size_t i(0); i < nbVanillaSwaps; ++i)
        {
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(OIS, TENOR), i));
            if(!description.empty())
            {
                const double swapRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(OIS, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(swapRate);
                ++(*instrumentIndex);
            }
        }
    }
     
    double OvernightIndexedSwap::getLastRelevantTime() const 
    {
        return ModuleDate::getYearsBetween(m_fixedLeg->getValueDate(), getEndDate()); 
    }
    
    const double OvernightIndexedSwap::computeModelPrice(const BaseModelPtr model) const
    {
        return m_fixedLeg->getValue(*model) * getRate() - m_floatingLeg->getValue(*model);
    }
        
    void OvernightIndexedSwap::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {
        // compute the fixed leg gradient and multiply it by the swap rate
        m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd);
        // subtract the floating leg gradient
        m_floatingLeg->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd);
    }

	void OvernightIndexedSwap::accumulateGradient(BaseModel const& baseModel,
											 double multiplier,
											 GradientIterator gradientBegin,
											 GradientIterator gradientEnd,
											 const CurveTypeConstPtr& curveType)
	{
		// compute the fixed leg gradient and multiply it by the swap rate
        m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd, curveType);
        // subtract the floating leg gradient
        m_floatingLeg->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd, curveType);
	}
	
	void OvernightIndexedSwap::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void OvernightIndexedSwap::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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

	void OvernightIndexedSwap::update()
    {
        m_floatingLeg->update();
        m_fixedLeg->update();
    }

	double OvernightIndexedSwap::getVariableInitialGuess(const double flowTime,
													 const BaseModel* const model) const
	{
		LT_LOG << "initial guess for OIS" << getDescription().string()<< ", " << CurveType::Discount()->getDescription() << " (fxd tenor: " << m_fixedLegTenor << ")" << endl;
		return model->getVariableValueFromSpineDiscountFactor(flowTime, 1.0/pow(1.0 + getRate(), flowTime), CurveType::Discount());
	}

	double OvernightIndexedSwap::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return m_fixedLeg->getValue(*model);
	}

	void OvernightIndexedSwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
		m_fixedLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd);
	}

	void OvernightIndexedSwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		m_fixedLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd, curveType);
	}

	double OvernightIndexedSwap::computeParRate(const BaseModelPtr& model)
	{
		return m_floatingLeg->getValue(*model) / m_fixedLeg->getValue(*model);
	}
	
	void OvernightIndexedSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                            const BaseModel& model,
										    const double multiplier, 
										    IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		m_fixedLeg->fillRepFlows(assetDomain, model, multiplier * getRate(), fundingRepFlows);
		m_floatingLeg->fillRepFlows(assetDomain, model, -multiplier, fundingRepFlows);
	}

	void OvernightIndexedSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                            const BaseModel& model,
										    const double multiplier, 
										    IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		//	Note: no index rep flows for fixed leg
		m_floatingLeg->fillRepFlows(assetDomain, model, -multiplier, indexRepFlows);
	}

	double OvernightIndexedSwap::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<OvernightIndexedSwap>(), IDeA_KEY(OIS, TENOR), IDeA_KEY(OIS, RATE));
	}

    ostream& OvernightIndexedSwap::print(ostream& out) const
    {
        out << "OIS" << getDescription().string();
        return out;
    }

    /**
        @brief Clone this OIS.

        Uses a lookup to maintain directed graph relationships.

        @param lookup A lookup of previuosly created clones.
        @return       The clone of this instance.
    */
    ICloneLookupPtr OvernightIndexedSwap::cloneWithLookup(CloneLookup& lookup) const
    {
        return CalibrationInstrumentPtr(new OvernightIndexedSwap(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    OvernightIndexedSwap::OvernightIndexedSwap(OvernightIndexedSwap const& original, CloneLookup& lookup) : 
        CalibrationInstrument(original),
        m_details(original.m_details),
        m_fixedLegTenor(original.m_fixedLegTenor),
        m_floatingLegTenor(original.m_floatingLegTenor),
        m_fixedLeg(lookup.get(original.m_fixedLeg)),
        m_floatingLeg(lookup.get(original.m_floatingLeg))
    {
    }

	void OvernightIndexedSwap::fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const
	{
		cashFlowsTable.set<LTQuant::GenericDataPtr>("Fixed Leg", 0, m_fixedLeg->getCashFlows());
		cashFlowsTable.set<LTQuant::GenericDataPtr>("Floating Leg", 0, m_floatingLeg->getCashFlows());
	}

	void OvernightIndexedSwap::fillCashFlowPVsTable(const BaseModel& model, LTQuant::GenericData& cashFlowPVsTable) const
	{
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Fixed Leg", 0, m_fixedLeg->computeCashFlowPVs(model));
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Floating Leg", 0, m_floatingLeg->computeCashFlowPVs(model));
	}
}   // FlexYCF