#include "stdafx.h"

// FlexYCF
#include "FxForward.h"
#include "CalibrationInstruments.h"
#include "BaseModel.h"
#include "ForwardRate.h"
#include "GlobalComponentCache.h"
#include "TenorUtils.h"
#include "DataExtraction.h"
#include "FlexYcfUtils.h"
#include "FlexYCFCloneLookup.h"

// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "MarketConvention.h"
#include "TradeConventionCalcH.h"

// QuantCore
#include "DayCount.h"

// LTQuantLib
#include "Data/GenericData.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "DateUtils.h"
#include "ModuleDate/InternalInterface/IRFunctionalInterface.h"
#include "ModuleDate/InternalInterface/FXFunctionalInterface.h"

using namespace std;
using namespace LTQC;
using namespace IDeA;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<FxForwardInstrument>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, FXFORWARD);
	}

    using namespace LTQuant;

    FxForwardInstrument::FxForwardInstrument(const string& description,
								   const LT::date spotDate,
                                   const LT::date deliveryDate,
                                   const double rate,
                                   const LT::date valueDate,
								   const LT::Str& fxIndex,
								   const IDeA::FxQuoteType quoteType,
					               double scaling,
                                   double valueDateRate,
								   const IDeA::FxForwardMktConvention& tradeDetails,
                                   GlobalComponentCache& globalComponentCache) :
        CalibrationInstrument(rate, getKeyName<FxForwardInstrument>(), description, valueDate, spotDate, deliveryDate),
		m_deliveryDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, deliveryDate))),
		m_spotDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, spotDate))),
		m_fxIndex(fxIndex),
		m_quoteType(quoteType),
		m_scaling(scaling),
        m_valueDateRate(valueDateRate)
		
    {

        IDeA::FxSpot::getCurrencies(m_domCurrency, m_forCurrency, fxIndex);
        m_initialized =  false;
    }

	FxForwardInstrument::FxForwardInstrument(const std::string& tenorDescription,
								   const LT::date spotDate,
								   const LT::date deliveryDate,
                                   const double rate,
                                   const LT::date valueDate,
								   const LT::Str& fxIndex,
								   const IDeA::FxQuoteType quoteType,
					               double scaling,
                                   double valueDateRate,
								   const IDeA::FxForwardMktConvention& tradeDetails) :
        CalibrationInstrument(rate, getKeyName<FxForwardInstrument>(), tenorDescription, valueDate, spotDate, deliveryDate),
		m_deliveryDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, deliveryDate))),
		m_spotDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, spotDate))),
		m_fxIndex(fxIndex),
		m_quoteType(quoteType),
		m_scaling(scaling),
        m_valueDateRate(valueDateRate)
    {
		IDeA::FxSpot::getCurrencies(m_domCurrency, m_forCurrency, fxIndex);
	    m_initialized =  false;
    }
    
	FxForwardInstrument::~FxForwardInstrument()
    {
    }

    void FxForwardInstrument::initialize(const BaseModel& baseModel) const
    {
        //m_spotRate = getFxSpotDomToFor(m_domCurrency, m_forCurrency, fxSpots, yieldCurves);
        m_spotRate = baseModel.getDependentFXRate(FXSpotAssetDomain(m_forCurrency, m_domCurrency));
        m_valueDateRate = m_spotRate - m_valueDateRate;

        // iterate through the dependent marketdata on the base model to identify the index
        LT::Str market;
        if( !baseModel.getDependentMarketData() )
        {
            LTQC_THROW( IDeA::ModelException, "FxForwardInstrument: unable to find any dependencies");
        }
        for (size_t i = 1; i < baseModel.getDependentMarketData()->table->rowsGet(); ++i) 
        {
            AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
            LT::Str asset = IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
            if (adType == IDeA::AssetDomainType::IR )
            {
                if((asset.compareCaseless(m_forCurrency) == 0) || (asset.compareCaseless(m_domCurrency) == 0))
                {
                    market = IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1); 
                    break;
                }
            }
        }
        
        if(baseModel.hasDependentModel(IRAssetDomain(m_forCurrency, market)))
        {
            m_dependentModel = baseModel.getDependentModel(IRAssetDomain(m_forCurrency, market));
            m_dependentModelCcy = m_forCurrency;
        }
        else if(baseModel.hasDependentModel(IRAssetDomain(m_domCurrency, market)))
        {
            m_dependentModel = baseModel.getDependentModel(IRAssetDomain(m_domCurrency, market));
            m_dependentModelCcy = m_domCurrency;
        }
        else
        {
            LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model for " << m_domCurrency.data() << " or " << m_forCurrency.data());
        }

        m_initialized = true;
    }

	CalibrationInstrumentPtr FxForwardInstrument::create(const LTQuant::GenericData& instrumentParametersTable,
												    const LT::Ptr<LT::date>& buildDate,
													const LTQuant::GenericData& curveParametersTable)
	{
		
		LT::date spotDate, deliveryDate;
		const IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(curveParametersTable));
		const IDeA::FxForwardMktConvention& tradeDetails(conventions->m_fxForwards);
        ModuleDate::FXStaticData fxStaticData = tradeDetails.m_fxStaticData;
		
        IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, DOMESTICCALENDAR), fxStaticData.getCCY1CalendarName(), fxStaticData.getCCY1CalendarName());
        IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FOREIGNCALENDAR), fxStaticData.getCCY2CalendarName(), fxStaticData.getCCY2CalendarName());
		
        std::string spotDaysStr;
		if( IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPOTDAYS), spotDaysStr, spotDaysStr) )
		{
            if( !spotDaysStr.empty() )
            {
			    LTQC::Tenor spotDaysTenor = LTQC::Tenor(spotDaysStr);
			    fxStaticData.setSpotDays(spotDaysTenor.asDays());
            }
		}

		IDeA::FxQuoteType quoteType = IDeA::FxQuoteType(IDeA::extract<std::string>(instrumentParametersTable, IDeA_KEY(FXFORWARD, FXQUOTETYPE)));
		double scaling = 1.0;
		if( IDeA::FxQuoteType::OutRight != quoteType )
		{
			IDeA::permissive_extract<double>(instrumentParametersTable, IDeA_KEY(FXFORWARD, SCALING),scaling, 1.0);
		}

		if(buildDate)
		{
			const bool found(IDeA::permissive_extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE), spotDate, *buildDate));
			if (!found)
				spotDate = FxForwardInstrument::computeFxSpotDate(*buildDate, fxStaticData);
		}
		else
		{
			spotDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE));
		}
		deliveryDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE));

		std::string ccyPair = tradeDetails.m_ccyPair.string();
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, INDEX), ccyPair, ccyPair);
		return CalibrationInstrumentPtr(new FxForwardInstrument("", spotDate, deliveryDate, 0.0, *buildDate, ccyPair, quoteType, scaling, 0.0, tradeDetails));
	}

    void FxForwardInstrument::createInstruments(CalibrationInstruments& instrumentList, 
                                           LTQuant::GenericDataPtr instrumentTable,
                                           LTQuant::GenericDataPtr masterTable,
                                           GlobalComponentCache& globalComponentCache,
                                           const LTQuant::PriceSupplierPtr)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		
		//	Get the deposit rate market conventions
        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		const IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));
		const IDeA::FxForwardMktConvention& tradeDetails(conventions->m_fxForwards);
        
		
        double valueDateRate = 0.0;
		for(size_t i = 0; i < IDeA::numberOfRecords(*instrumentTable); ++i)
        {
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, TENOR), i));	
            
			if(!description.empty())
            {
                // Is the user specifying a short-dated FX forward e.g. O/N, T/N, S/N
                ModuleDate::ShortDateDuration::Type shortDateDuration;
                if (!ModuleDate::ShortDateDuration::tryFromString(description, shortDateDuration))
                {
                    shortDateDuration = static_cast<ModuleDate::ShortDateDuration::Type>(-1);
                }
                if ((shortDateDuration == ModuleDate::ShortDateDuration::Today) || (shortDateDuration == ModuleDate::ShortDateDuration::Tomorrow) || (shortDateDuration == ModuleDate::ShortDateDuration::Spot))
                {
                        LT_THROW_ERROR("FXForward: cannot use FX dates such as 'TODAY', 'TOMORROW' or 'SPOT'");
                }
                
                ModuleDate::FXStaticData fxStaticData = tradeDetails.m_fxStaticData;
                IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, DOMESTICCALENDAR), i, fxStaticData.getCCY1CalendarName(), fxStaticData.getCCY1CalendarName());
                IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, FOREIGNCALENDAR), i, fxStaticData.getCCY2CalendarName(), fxStaticData.getCCY2CalendarName());
                ModuleDate::CalendarPtr cal1 = ModuleDate::CalendarFactory::create(fxStaticData.getCCY1CalendarName().c_str());
		        ModuleDate::CalendarPtr cal2 = ModuleDate::CalendarFactory::create(fxStaticData.getCCY2CalendarName().c_str());

				double rate(0.0);
				bool hasOvernightNext = true, hasTomNext = true;
				LT::date fxSpotDate, spotDate, deliveryDate; 

				if (shortDateDuration == ModuleDate::ShortDateDuration::Overnight || shortDateDuration == ModuleDate::ShortDateDuration::TomorrowNext)
				{
					hasOvernightNext = !cal1->isHoliday(valueDate) && !cal2->isHoliday(valueDate);
					if (hasOvernightNext)
					{
						bool defined = false;
                        ModuleDate::FXStaticData fxStaticDataZeroSpotDays = fxStaticData;
                        fxStaticDataZeroSpotDays.setSpotDays(0);
						ModuleDate::getFXShortDates(valueDate, fxStaticDataZeroSpotDays, ModuleDate::ShortDateDuration::Overnight, spotDate, deliveryDate, defined);
						fxSpotDate = FxForwardInstrument::computeFxSpotDate(valueDate, fxStaticData);
						hasTomNext = (deliveryDate != fxSpotDate);
					}
					else
						hasTomNext = true;
				}

                std::string defaultQuoteType("SpreadPremium"), quoteTypeString;
				IDeA::FxQuoteType quoteType = IDeA::FxQuoteType(defaultQuoteType);
				bool foundQuoteType = IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, FXQUOTETYPE), i, quoteTypeString, defaultQuoteType);
                if( foundQuoteType )
                {
                     quoteType = IDeA::FxQuoteType(quoteTypeString);
                }

              
                double scaling = 1.0;
				if( IDeA::FxQuoteType::OutRight != quoteType )
				{
					IDeA::permissive_extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, SCALING), i, scaling, 1.0);
				}

				std::string spotDaysStr;
				if ( IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, SPOTDAYS), i, spotDaysStr, spotDaysStr) )
				{
                    if( !spotDaysStr.empty() )
                    {
					    LTQC::Tenor spotDaysTenor = LTQC::Tenor(spotDaysStr);
					    fxStaticData.setSpotDays(spotDaysTenor.asDays());
                    }
				}

				std::string ccyPair = tradeDetails.m_ccyPair.string();
				IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, INDEX), ccyPair, ccyPair);
				
                if(shortDateDuration == ModuleDate::ShortDateDuration::Overnight)
                {
					if (!hasOvernightNext) continue;	// break for loop to proceed next instrument

					rate = IDeA::extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, RATE), i);

                    if( LTQC::FxQuoteType::SpreadPremium == quoteType )
                    {
                        valueDateRate = rate/scaling;
                    }
                    else if( LTQC::FxQuoteType::SpreadDiscount == quoteType )
                    {
                        valueDateRate = -rate/scaling;
                    }
                     
					const bool TNquoteNeeded = (fxSpotDate > deliveryDate) && hasTomNext;
                    if(  TNquoteNeeded && i+1 < IDeA::numberOfRecords(*instrumentTable) )
                    {
                        const string tnStr(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, TENOR), i+1));
                        ModuleDate::ShortDateDuration::Type tn;
                        ModuleDate::ShortDateDuration::tryFromString(tnStr, tn);
                        if( tn == ModuleDate::ShortDateDuration::TomorrowNext )
                        {
                             const double TNrate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, RATE), i+1));
                             double TNscaling = 1.0;
                             IDeA::permissive_extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, SCALING), i+1, TNscaling, 1.0);
                             if( LTQC::FxQuoteType::SpreadPremium == quoteType )
                             {  
                                valueDateRate += TNrate/TNscaling;
                             }
                             else if( LTQC::FxQuoteType::SpreadDiscount == quoteType )
                             {
                                valueDateRate -= TNrate/TNscaling;
                             }
                             else
                             {
                                LT_THROW_ERROR("FxForwardInstrument:: quote type outright/discount not implemented for O/N")
                             }
                        }
                        else
                        {
                            LT_THROW_ERROR("FxForward: given O/N quote, the next one should be  T/N")
                        }
                    }
                    else
                    { 
                        if( TNquoteNeeded )
                            LT_THROW_ERROR("FxForward: given O/N quote, the next one should be  T/N")
                    }

                }
                else if(shortDateDuration == ModuleDate::ShortDateDuration::TomorrowNext)
                {
					if (!hasTomNext) continue;	// break for loop to proceed next instrument

					rate = IDeA::extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, RATE), i);

                    bool tnDefined = false;
                    ModuleDate::FXStaticData fxStaticDataOneSpotDay = fxStaticData;
                    fxStaticDataOneSpotDay.setSpotDays(1);
                    ModuleDate::getFXShortDates(valueDate, fxStaticDataOneSpotDay, ModuleDate::ShortDateDuration::TomorrowNext, spotDate, deliveryDate, tnDefined);
                   
					double ONrate = 0.0, ONscaling = 1.0;

					if (hasOvernightNext)
					{
						// O/N quote is needed
						if( i > 0 )
						{
							const string onStr(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, TENOR), i - 1));
							ModuleDate::ShortDateDuration::Type on;
							ModuleDate::ShortDateDuration::tryFromString(onStr, on);
							if( on == ModuleDate::ShortDateDuration::Overnight)
							{
								ONrate = IDeA::extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, RATE), i - 1);
								IDeA::permissive_extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, SCALING), i - 1,ONscaling, 1.0);
							}
							else
							{
								LT_THROW_ERROR("FxForward: given T/N quote, the previous one should be  O/N")
							}
						}
						else
						{
							LT_THROW_ERROR("FxForward: given T/N quote, the previous one should be  O/N")
						}
					}

					if( LTQC::FxQuoteType::SpreadDiscount == quoteType )
					{
						valueDateRate = - rate/scaling - ONrate/ONscaling;
					}
					else if(  LTQC::FxQuoteType::SpreadPremium == quoteType )
					{
						valueDateRate =  rate/scaling + ONrate/ONscaling;
					}
					else
					{
						LT_THROW_ERROR("FxForward: quote type outright not implemented for T/N")
					}                    
                }
                else
                {   
                    // Long-dated FX forwards
					rate = IDeA::extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, RATE), i);
                    ModuleDate::CompositeDuration const duration = ModuleDate::CompositeDuration::fromString(description.data());
                    ModuleDate::getFXLongDates(valueDate, fxStaticData, duration, spotDate, deliveryDate);
                }
                CalibrationInstrumentPtr instrument( new FxForwardInstrument( description, spotDate, deliveryDate, rate, valueDate, ccyPair, quoteType, scaling, valueDateRate, tradeDetails, globalComponentCache) );
                instrumentList.add(instrument);
            }
        }
    }

    void FxForwardInstrument::updateInstruments(CalibrationInstruments& instrumentList, 
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
            const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(FXFORWARD, TENOR), i));
			if(!description.empty())
            {
                const double rate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(FXFORWARD, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(rate);
                ++(*instrumentIndex);
            }
        }
    }

    double FxForwardInstrument::getLastRelevantTime() const
    {
		return m_deliveryDiscountFactor->getArguments().getFlowTime();
    }
    
    double FxForwardInstrument::getTenor() const
    {
		LT_THROW_ERROR("FxForwardInstrument::getTenor: not implemented")
        return 0.0;
    }

    const double FxForwardInstrument::computeModelPrice(const BaseModelPtr model) const
    {
        if(!m_initialized)
        {
		    initialize(*model);
        }
        double spotRate = m_spotRate;
        const LT::Str& description = getDescription();
        if(  description.compareCaseless("O/N") == 0 )
        {
            spotRate = m_valueDateRate;
        }
        if( description.compareCaseless("T/N") == 0 )
        {
            if( IDeA::FxQuoteType::SpreadDiscount == m_quoteType )
            {
                spotRate +=  getRate()/m_scaling;
            } 
            else if( IDeA::FxQuoteType::SpreadPremium == m_quoteType )
            {
                spotRate -=  getRate()/m_scaling;
            }
            else
            {
                LT_THROW_ERROR("FxForwardInstrument::computeModelPrice: should not be here")
            }
        }

        if( m_forCurrency.compareCaseless(m_dependentModelCcy) == 0 )
        {
            return computeParRate(model, m_dependentModel, m_domCurrency, m_forCurrency, spotRate, m_domCurrency, m_forCurrency);
        }
        if( m_domCurrency.compareCaseless(m_dependentModelCcy) == 0 )
        {
            return computeParRate(model, m_dependentModel, m_forCurrency, m_domCurrency, spotRate, m_domCurrency, m_forCurrency);
        }
        LT_THROW_ERROR("FxForwardInstrument::computeModelPrice: should not be here")
    }

    const double FxForwardInstrument::computePV(const BaseModelPtr model) const
    {
		LT_THROW_ERROR("FxForwardInstrument::computePV: not implemented")
        return 0.0;
    }

    const double FxForwardInstrument::computeBPV(const BaseModelPtr model) const
    {
		return - oneBasisPoint( ) * model->getDiscountFactor( getLastRelevantTime() );
    }

    void FxForwardInstrument::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {   
        double spotDf =  m_spotDiscountFactor->getValue(baseModel);
        double deliveryDf =  m_deliveryDiscountFactor->getValue(baseModel);
        
        double forDeliveryDf = m_deliveryDiscountFactor->getValue(*m_dependentModel);
        double forSpotDf = m_spotDiscountFactor->getValue(*m_dependentModel);

        if( m_domCurrency.compareCaseless(m_dependentModelCcy) == 0 )
        {   
            double spotTimesForDfs = m_spotRate * forSpotDf / forDeliveryDf;
            m_spotDiscountFactor->accumulateGradient(baseModel, - multiplier * deliveryDf/(spotDf * spotDf) * spotTimesForDfs, gradientBegin, gradientEnd);
            m_deliveryDiscountFactor->accumulateGradient(baseModel, multiplier/spotDf * spotTimesForDfs, gradientBegin, gradientEnd);
        }
        else
        {
            double spotTimesForDfs = m_spotRate * forDeliveryDf / forSpotDf;
            m_deliveryDiscountFactor->accumulateGradient(baseModel, - multiplier * spotDf/(deliveryDf * deliveryDf) * spotTimesForDfs, gradientBegin, gradientEnd);
            m_spotDiscountFactor->accumulateGradient(baseModel, multiplier/deliveryDf * spotTimesForDfs, gradientBegin, gradientEnd);
        }
    }

	void FxForwardInstrument::accumulateGradient(BaseModel const& baseModel,
											double multiplier,
											GradientIterator gradientBegin,
											GradientIterator gradientEnd,
											const CurveTypeConstPtr& curveType)
	{
        double spotDf =  m_spotDiscountFactor->getValue(baseModel);
        double deliveryDf =  m_deliveryDiscountFactor->getValue(baseModel);

        double forDeliveryDf = m_deliveryDiscountFactor->getValue(*m_dependentModel);
        double forSpotDf = m_spotDiscountFactor->getValue(*m_dependentModel);
        
        if( m_domCurrency.compareCaseless(m_dependentModelCcy) == 0 )
        {   
            double spotTimesForDfs = m_spotRate * forSpotDf / forDeliveryDf;
            m_spotDiscountFactor->accumulateGradient(baseModel, - multiplier * deliveryDf/(spotDf * spotDf) *  spotTimesForDfs, gradientBegin, gradientEnd, curveType);
            m_deliveryDiscountFactor->accumulateGradient(baseModel, multiplier/spotDf *  spotTimesForDfs, gradientBegin, gradientEnd, curveType);
        }
        else
        {   
            double spotTimesForDfs = m_spotRate * forDeliveryDf / forSpotDf;
            m_deliveryDiscountFactor->accumulateGradient(baseModel, - multiplier * spotDf/(deliveryDf * deliveryDf) * spotTimesForDfs, gradientBegin, gradientEnd, curveType);
            m_spotDiscountFactor->accumulateGradient(baseModel, multiplier/deliveryDf * spotTimesForDfs, gradientBegin, gradientEnd, curveType);
        } 
	}
	
	void FxForwardInstrument::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void FxForwardInstrument::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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

	void FxForwardInstrument::update()
    {
        m_deliveryDiscountFactor->update();
		m_spotDiscountFactor->update();
    }

	double FxForwardInstrument::getVariableInitialGuess(const double flowTime,
												   const BaseModel* const model) const
	{   
        if( !m_initialized )
        {
            initialize(*model);
        }

        const LT::Str& description = getDescription();
        if( description.compareCaseless("T/N") == 0 )
        {
            double fwd = m_spotRate/m_valueDateRate;
            double df  = m_dependentModel->getDiscountFactor(flowTime);
            if( m_domCurrency.compareCaseless(m_dependentModelCcy) != 0 )
            {
                fwd = 1.0/fwd;
            }
            return model->getVariableValueFromSpineDiscountFactor(flowTime, fwd * df, CurveType::_3M());
        }
        if( description.compareCaseless("O/N") == 0 )
        {
            double fwd = fxForwardRate(m_valueDateRate,m_domCurrency,m_forCurrency)/m_valueDateRate;
            double df  = m_dependentModel->getDiscountFactor(flowTime);
            if( m_domCurrency.compareCaseless(m_dependentModelCcy) != 0 )
            {
                fwd = 1.0/fwd;
            }
            return model->getVariableValueFromSpineDiscountFactor(flowTime, fwd * df, CurveType::_3M());
        }
        
        double fwdOverSpot = fxForwardRate(m_spotRate,m_domCurrency,m_forCurrency)/m_spotRate;
        if( m_domCurrency.compareCaseless(m_dependentModelCcy) != 0 )
        {
            fwdOverSpot = 1.0/fwdOverSpot;
        }
        double forDf     = m_deliveryDiscountFactor->getValue(*m_dependentModel);
        double forSpotDf = m_spotDiscountFactor->getValue(*m_dependentModel);
        double dfOverDfSpot = forDf/forSpotDf;
        double domSpotDf    = m_spotDiscountFactor->getValue(*model);
        const double& spotFlowTime = m_spotDiscountFactor->getArguments().getFlowTime();
        domSpotDf = (domSpotDf > 0.9 && domSpotDf < 1.1 ? domSpotDf : pow(fwdOverSpot * dfOverDfSpot, spotFlowTime/(flowTime - spotFlowTime) ) );
        double dfGuess = fwdOverSpot * dfOverDfSpot * domSpotDf;
        if( dfGuess  <= 0.0 || !_finite(dfGuess) )
        {
            LT_THROW_ERROR("FxForward: can not guess discount factor for time " << flowTime << " : " << dfGuess) 
        }
        return model->getVariableValueFromSpineDiscountFactor(flowTime, dfGuess, CurveType::_3M());
	}

	double FxForwardInstrument::calculateRateDerivative(const BaseModelPtr& /* model */) const
	{
		return -1.0;
	}
    
	void FxForwardInstrument::accumulateRateDerivativeGradient(const BaseModel& model,
														  double multiplier,
														  GradientIterator gradientBegin,
														  GradientIterator gradientEnd) const
	{ 
	}

	void FxForwardInstrument::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		LT_THROW_ERROR("FxForwardInstrument::accumulateRateDerivativeGradient: not implemented") 
	}

	double FxForwardInstrument::computeParRate(const BaseModelPtr& model)
	{
		
		LT_THROW_ERROR("FxForwardInstrument::computeParRate: not implemented") 
		return 0;
	}
	double FxForwardInstrument::computeParRate(const BaseModelConstPtr& domModel, const BaseModelConstPtr& forModel, const LT::Str& domModelCcy, const LT::Str& forModelCcy, double spotFX, const LT::Str& domCcyFxSpot, const LT::Str& forCcyFxSpot) const
	{
		if( m_fxIndex.compareCaseless(forModelCcy.string() + domModelCcy.string()) != 0 && m_fxIndex.compareCaseless(domModelCcy.string() + forModelCcy.string()) != 0)
		{
			LT_THROW_ERROR("FxForwardInstrument::computeParRate for "  + m_fxIndex.string() + " can not be computed with models for " + forModelCcy.string() + " and " + domModelCcy.string()) 
		}

		double domDf = m_deliveryDiscountFactor->getValue(*domModel);
		double forDf = m_deliveryDiscountFactor->getValue(*forModel);
		
		double domSpotDf = m_spotDiscountFactor->getValue(*domModel);
		double forSpotDf = m_spotDiscountFactor->getValue(*forModel);
		
		if( m_domCurrency.compareCaseless(domModelCcy) == 0 &&  m_forCurrency.compareCaseless(forModelCcy) == 0 )
		{
			if( m_domCurrency.compareCaseless(domCcyFxSpot) == 0 &&  m_forCurrency.compareCaseless(forCcyFxSpot) == 0 )
			{
				return spotFX * (forDf / domDf) * (domSpotDf / forSpotDf);
			}
			if( m_domCurrency.compareCaseless(forCcyFxSpot) == 0 &&  m_forCurrency.compareCaseless(domCcyFxSpot) == 0 )
			{
				return 1.0/spotFX * (forDf / domDf) * (domSpotDf / forSpotDf);
			}
		}
		if( m_domCurrency.compareCaseless(forModelCcy) == 0 &&  m_forCurrency.compareCaseless(domModelCcy) == 0 )
		{	
			if( m_domCurrency.compareCaseless(domCcyFxSpot) == 0 &&  m_forCurrency.compareCaseless(forCcyFxSpot) == 0 )
			{	
				return spotFX * (domDf / forDf) * (forSpotDf / domSpotDf);
			}
			if( m_domCurrency.compareCaseless(forCcyFxSpot) == 0 &&  m_forCurrency.compareCaseless(domCcyFxSpot) == 0 )
			{	
				return 1.0/spotFX * (domDf / forDf) * (forSpotDf / domSpotDf);
			}
		}
		LT_THROW_ERROR("FxForwardInstrument::computeParRate needs fxSpot for "  + m_fxIndex.string() + " but " + forCcyFxSpot.string() + domCcyFxSpot.string() + " provided") 
	}
    
	double FxForwardInstrument::fxForwardRate(double spotRate, const LT::Str& domCcyFxSpot, const LT::Str& forCcyFxSpot) const
	{
		double spot = 0.0;
		if( m_domCurrency.compareCaseless(domCcyFxSpot) == 0 &&  m_forCurrency.compareCaseless(forCcyFxSpot) == 0 )
		{
			spot = spotRate;
		} 
		else if( m_domCurrency.compareCaseless(forCcyFxSpot) == 0 &&  m_forCurrency.compareCaseless(domCcyFxSpot) == 0 )
		{
			spot = 1.0/spotRate;
		}
		else
		{
			LT_THROW_ERROR("FxForwardInstrument::fxForwardRate needs fxSpot for "  + m_fxIndex.string() + " but " + forCcyFxSpot.string() + domCcyFxSpot.string() + " provided") 
		}
        
        const LT::Str& description = getDescription();
        if( description.compareCaseless("O/N") == 0)
        {
            switch(m_quoteType)
		    {	
                // signs are switched!
			    case IDeA::FxQuoteType::SpreadPremium:
				    return spot + getRate()/m_scaling;
			
			    case IDeA::FxQuoteType::SpreadDiscount:
				    return spot - getRate()/m_scaling;
			
			    default:
				    LT_THROW_ERROR("FxForwardInstrument::feForwardRate: unknown quote type"); 
		    }
        }
        if(description.compareCaseless("T/N") == 0)
        {
            return spot;
        }

		switch(m_quoteType)
		{
			case IDeA::FxQuoteType::OutRight:
				return getRate();
			
			case IDeA::FxQuoteType::SpreadPremium:
				return spot + getRate()/m_scaling;
			
			case IDeA::FxQuoteType::SpreadDiscount:
				return spot - getRate()/m_scaling;
			
			default:
				LT_THROW_ERROR("FxForwardInstrument::feForwardRate: unknown quote type"); 
		}
	}

	void FxForwardInstrument::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                           const BaseModel& model,
									       const double multiplier, 
									       IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		LT_THROW_ERROR("FxForwardInstrument::fillRepFlows: not implemented") 
	}
	
	
	double FxForwardInstrument::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<FxForwardInstrument>(), IDeA_KEY(FXFORWARD, TENOR), IDeA_KEY(FXFORWARD, RATE));
	}
	

	ostream& FxForwardInstrument::print(ostream& out) const
    {
        out << "FxFwd" << getDescription().string();
        return out;
    }

    void FxForwardInstrument::finishCalibration(const BaseModelPtr model)
    {
        //FX fowards cannot compute their PVs or BPVs if they are ever
        //used the exception will be thrown at time of use
        if(!m_flowsRemoved)
        {
            //don't care about valuation exceptions. The value will
            //stay uninitilised and we will throw on first use of it
            try
            {
                setRateDerivative(calculateRateDerivative(model));
            }
            catch(...)
            {}
            m_flowsRemoved=true;
        }
        m_deliveryDiscountFactor.reset();
        m_spotDiscountFactor.reset();
    }

    void FxForwardInstrument::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
        const FxForwardInstrumentPtr ourTypeSrc=std::tr1::static_pointer_cast<FxForwardInstrument>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        m_deliveryDiscountFactor=ourTypeSrc->m_deliveryDiscountFactor;
        m_spotDiscountFactor=ourTypeSrc->m_spotDiscountFactor;

        CalibrationInstrument::reloadInternalState(src);
    }

    ICloneLookupPtr FxForwardInstrument::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FxForwardInstrument(*this, lookup));
    }

    FxForwardInstrument::FxForwardInstrument(const FxForwardInstrument& original, CloneLookup& lookup) :
        CalibrationInstrument(original),
        m_deliveryDiscountFactor(lookup.get(original.m_deliveryDiscountFactor)),
        m_spotDiscountFactor(lookup.get(original.m_spotDiscountFactor)),
        m_fxIndex(original.m_fxIndex),
        m_domCurrency(original.m_domCurrency),
        m_forCurrency(original.m_forCurrency),
        m_quoteType(original.m_quoteType),
        m_scaling(original.m_scaling),
        m_initialized(original.m_initialized),
        m_spotRate(original.m_spotRate),
        m_valueDateRate(original.m_valueDateRate),
        m_dependentModel(original.m_dependentModel),
        m_dependentModelCcy(original.m_dependentModelCcy)
    {
    }


}
