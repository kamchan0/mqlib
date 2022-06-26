#include "stdafx.h"
#include "CommodityFutures.h"
#include "BaseModel.h"
#include "Data/GenericData.h"
#include "Instruments/Index.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "ForwardRate.h"
#include "GlobalComponentCache.h"
#include "Pricers/PriceSupplier.h"
#include "ModuleDate/InternalInterface/Utils.h"
#include "DataExtraction.h"
#include "DateUtils.h"
#include "FlexYcfUtils.h"
#include "ModuleDate/InternalInterface/CalendarFactory.h"
#include "FlexYCFCloneLookup.h"

// IDeA
#include "DictYieldCurve.h"
#include "MarketConvention.h"
#include "TradeConventionCalcH.h"
#include "DictionaryManager.h"
#include "Exception.h"

// QuantCore
#include "utils\DayCount.h"
#include "QCException.h"


using namespace LTQC;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<CommodityFutures>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, COMMODITYFUTURES);
	}


    using namespace LTQuant;
    using namespace ModuleDate;

   
    CommodityFutures::CommodityFutures(const std::string dateDescription,
                     const LT::date valueDate,
                     const LT::date fixingDate, 
                     const LT::date startDate, 
                     const LT::date endDate, 
                     const double quotedPrice, 
                     const double convexityAdjustment,
                     const double contractSize,
                     const std::string& fxPair,
                     GlobalComponentCache& globalComponentCache):
		CalibrationInstrument(quotedPrice, getKeyName<CommodityFutures>(), dateDescription, fixingDate, startDate, endDate),
        m_forwardPrice(quotedPrice - convexityAdjustment),
        m_convexityAdjustment(convexityAdjustment),
        m_deliveryTime( endDate < valueDate ? 0.0 : getYearsBetween(valueDate, endDate)),
        m_contractSize( contractSize ),
        m_commodityCurrencyPair( fxPair ),
        m_deliveryDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate < valueDate ? valueDate : endDate)))
    {  
    }

    CommodityFutures::CommodityFutures(const std::string dateDescription,
                     const LT::date valueDate,
                     const LT::date fixingDate, 
                     const LT::date startDate, 
                     const LT::date endDate, 
                     const double quotedPrice, 
                     const double convexityAdjustment,
                     const double contractSize,
                     const std::string& fxPair):
        CalibrationInstrument(quotedPrice, getKeyName<CommodityFutures>(), dateDescription, fixingDate, startDate, endDate),
        m_forwardPrice(quotedPrice - convexityAdjustment),
        m_convexityAdjustment(convexityAdjustment),
        m_deliveryTime( endDate < valueDate ? 0.0 : getYearsBetween(valueDate, endDate)),
        m_contractSize( contractSize ),
        m_commodityCurrencyPair( fxPair ),
        m_deliveryDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate < valueDate ? valueDate : endDate)))
    {
    }

	CalibrationInstrumentPtr CommodityFutures::create(const LTQuant::GenericData& instrumentParametersTable, 
											 const LT::Ptr<LT::date>& buildDate,
											 const LTQuant::GenericData& curveParametersTable)
	{
        if( !buildDate )
        {
            LTQC_THROW( IDeA::MarketException, "To create a futures instrument, one needs to provide a build date" );
        }
        const LT::date valueDate( *buildDate );
        const std::string description( IDeA::extract< std::string >( instrumentParametersTable, IDeA_KEY(COMMODITYFUTURE, EXPIRY) ) );
        
        std::string calendar, deliveryOffset;
        IDeA::permissive_extract<std::string>( instrumentParametersTable, IDeA_KEY(COMMODITYFUTURE, CALENDAR), calendar, "NY" );
		CalendarPtr futuresCalendar( CalendarFactory::create( calendar) );
        IDeA::permissive_extract<std::string>( instrumentParametersTable, IDeA_KEY(COMMODITYFUTURE, DELIVERYOFFSET), deliveryOffset, "3b" );
        
        LT::date startDate = valueDate, endDate = deliveryDate( description, futuresCalendar, deliveryOffset);
        double convexityAdjustment( 0.0 );
        
        IDeA::permissive_extract< LT::date >( instrumentParametersTable, IDeA_KEY(COMMODITYFUTURE, STARTDATE), startDate, startDate );
        IDeA::permissive_extract< LT::date >( instrumentParametersTable, IDeA_KEY(COMMODITYFUTURE, ENDDATE), endDate, endDate );
        IDeA::permissive_extract< double >( instrumentParametersTable, IDeA_KEY(COMMODITYFUTURE, CONVEXITY), convexityAdjustment, 0.0 );
        std::string asset = IDeA::extract< std::string >( instrumentParametersTable, IDeA_KEY(COMMODITYFUTURE, ASSET) );
        std::string ccy   = IDeA::extract< std::string >( instrumentParametersTable, IDeA_KEY(COMMODITYFUTURE, CURRENCY) );

        std::string fxPair(asset + ccy);

      
        const double contractSize = 100.0;
		
        return CalibrationInstrumentPtr( new CommodityFutures( description,
                                                      valueDate,
												      startDate,
                                                      startDate,
                                                      endDate,
                                                      0.0,
                                                      convexityAdjustment,
                                                      contractSize,
                                                      fxPair)
                                        );
	}

    void CommodityFutures::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_deliveryDiscountFactor.reset();
    }
    void CommodityFutures::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
         const CommodityFuturesPtr ourTypeSrc=std::tr1::static_pointer_cast<CommodityFutures>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        m_deliveryDiscountFactor=ourTypeSrc->m_deliveryDiscountFactor;

        CalibrationInstrument::reloadInternalState(src);
    }
    void CommodityFutures::createInstruments(CalibrationInstruments& instruments, 
                                    LTQuant::GenericDataPtr instrumentTable, 
                                    LTQuant::GenericDataPtr masterTable,
                                    GlobalComponentCache& globalComponentCache,
                                    const LTQuant::PriceSupplierPtr priceSupplier)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));

        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
        const double contractSize = 100.0;

        for(size_t i(0); i < instrumentTable->numItems() - 1; ++i)
        {
			std::string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(COMMODITYFUTURE, EXPIRY), i));
            
			if(!description.empty())
            {
                std::string calendar, deliveryOffset;
                IDeA::permissive_extract<std::string>( *instrumentTable, IDeA_KEY(COMMODITYFUTURE, CALENDAR), i, calendar, "NY" );
		        CalendarPtr futuresCalendar( CalendarFactory::create( calendar) );
                IDeA::permissive_extract<std::string>( *instrumentTable, IDeA_KEY(COMMODITYFUTURE, DELIVERYOFFSET), i, deliveryOffset, "3b" );
               
                std::string asset(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(COMMODITYFUTURE, ASSET), i));
                std::string ccy(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(COMMODITYFUTURE, CURRENCY), i));
                std::string fxPair(asset + ccy);

                const double quotedPrice(IDeA::extract<double>(*instrumentTable, IDeA_KEY(COMMODITYFUTURE, PRICE), i));
                
                LT::date startDate, endDate = deliveryDate(description, futuresCalendar, deliveryOffset);
                IDeA::permissive_extract<LT::date>( *instrumentTable, IDeA_KEY(COMMODITYFUTURE, STARTDATE), i, startDate, valueDate );
                IDeA::permissive_extract<LT::date>( *instrumentTable, IDeA_KEY(COMMODITYFUTURE, ENDDATE), i, endDate, endDate );
				
                double convexityAdjustment(0.0);
                IDeA::permissive_extract<double>( *instrumentTable, IDeA_KEY(COMMODITYFUTURE, CONVEXITY), i, convexityAdjustment, 0.0 );
              
                CalibrationInstrumentPtr instrument( new CommodityFutures( description,
                                                                  valueDate,
																  startDate,
                                                                  startDate,
                                                                  endDate,
                                                                  quotedPrice,
                                                                  convexityAdjustment,
                                                                  contractSize,
                                                                  fxPair,
                                                                  globalComponentCache)
                                                   );
                instruments.add(instrument);
            }
        }
    }

     void CommodityFutures::updateInstruments(CalibrationInstruments& instrumentList, 
                                    LTQuant::GenericDataPtr instrumentTable, 
                                    size_t* instrumentIndex)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

        for(size_t i(0); i < instrumentTable->numItems() - 1; ++i)
        {
			const std::string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(COMMODITYFUTURE, EXPIRY), i));
            if(!description.empty())
            {
				const double price(IDeA::extract<double>(*instrumentTable, IDeA_KEY(COMMODITYFUTURE, PRICE), i));
                
                instrumentList[*instrumentIndex]->setRate(price);
                ++(*instrumentIndex);
            }
        }
    }   
   
    const double CommodityFutures::computeModelPrice(const BaseModelPtr model) const
    {
		return m_deliveryDiscountFactor->getValue(*model);
    }

    double CommodityFutures::getLastRelevantTime() const
    {
		return m_deliveryTime;
    }
    
    void CommodityFutures::accumulateGradient(BaseModel const& baseModel,
												double  multiplier,
												GradientIterator gradientBegin,
												GradientIterator gradientEnd)
    {
        m_deliveryDiscountFactor->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
    }
	
	void CommodityFutures::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void CommodityFutures::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
    void CommodityFutures::update()
    {
        m_deliveryDiscountFactor->update();
    }

	double CommodityFutures::calculateRateDerivative(const BaseModelPtr& /* model */) const
	{
		return 1.0;
	}

	void CommodityFutures::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
	}

	void CommodityFutures::accumulateRateDerivativeGradient(const BaseModel& model,
												  double multiplier,
												  GradientIterator gradientBegin,
												  GradientIterator gradientEnd,
												  const CurveTypeConstPtr& curveType) const
	{
		LT_THROW_ERROR( "Not implemented" );
	}

	double CommodityFutures::computeParRate(const BaseModelPtr& /* model */)
	{
		LT_THROW_ERROR( "Not implemented" );
	}

	double CommodityFutures::getDifferenceWithNewRate(const LTQuant::GenericData& /* instrumentListData */) const
	{
		LT_THROW_ERROR( "Not implemented" );
	}

   
    ICloneLookupPtr CommodityFutures::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new CommodityFutures(*this, lookup));
    }

    CommodityFutures::CommodityFutures(const CommodityFutures& original, CloneLookup& lookup)
        : CalibrationInstrument(original),
        m_futurePrice(original.m_futurePrice),
        m_forwardPrice(original.m_forwardPrice),
        m_convexityAdjustment(original.m_convexityAdjustment),
        m_deliveryTime(original.m_deliveryTime),
        m_contractSize(original.m_contractSize),
        m_commodityCurrencyPair(original.m_commodityCurrencyPair),
        m_deliveryDiscountFactor(lookup.get(original.m_deliveryDiscountFactor))
    {
    }

    LT::date CommodityFutures::deliveryDate(const std::string& description, const CalendarPtr& futuresCalendar, const std::string& EOMdeliveryOffset)
    {
        LT::date tmp = getIMMDate(description, futuresCalendar);
        LT::date endDate = tmp.end_of_month();
        endDate = ModuleDate::addDatePeriod(endDate, string("-") + EOMdeliveryOffset, futuresCalendar );
        return endDate;
    }

}   // FlexYCF
