#ifndef __LIBRARY_PRICERS_FLEXYCF_COMMODITYFUTURES_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_COMMODITYFUTURES_H_INCLUDED
#pragma once

#include "CalibrationInstruments.h"
#include "Gradient.h"
#include "Library/PublicInc/Date.h"
#include "DiscountFactor.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( PriceSupplier )
   
}
namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( Calendar )
}


namespace FlexYCF
{
	class GlobalComponentCache;

    class CommodityFutures : public CalibrationInstrument
    {
    public:
        CommodityFutures::CommodityFutures(const std::string dateDescription,
                     const LT::date valueDate,
                     const LT::date fixingDate, 
                     const LT::date startDate, 
                     const LT::date endDate, 
                     const double quotedPrice, 
                     const double convexityAdjustment,
                     const double contractSize,
                     const std::string& fxPair,
                     GlobalComponentCache& globalComponentCache);

        CommodityFutures::CommodityFutures(const std::string dateDescription,
                     const LT::date valueDate,
                     const LT::date fixingDate, 
                     const LT::date startDate, 
                     const LT::date endDate, 
                     const double quotedPrice, 
                     const double convexityAdjustment,
                     const double contractSize,
                     const std::string& fxPair);

        static std::string getName()
        {
            return "Commodity Futures";
        }

        CalibrationInstrumentPtr CommodityFutures::create(const LTQuant::GenericData& instrumentParametersTable, 
											 const LT::Ptr<LT::date>& buildDate,
											 const LTQuant::GenericData& curveParametersTable);

        static void createInstruments(CalibrationInstruments& instruments,
                                      LTQuant::GenericDataPtr instrumentTable,
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);
    
        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);

        virtual std::string getType() const
        {
            return CommodityFutures::getName();
        }

        virtual const double getMarketPrice() const
        {
            return getRate();
        }

        virtual double getLastRelevantTime() const;

        virtual const double computeModelPrice(const BaseModelPtr) const;
        
        virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd);

		// No concept of "curve type"
		virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr&)
		{
			accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
		}

		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);
        virtual void finishCalibration(const BaseModelPtr model);

        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src);
        /// Updates the instrument
        virtual void update();
        
		//	NOT IMPLEMENTED:
		virtual double calculateRateDerivative(const BaseModelPtr& model) const;
		virtual void accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const;
		virtual void accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const;
		virtual double computeParRate(const BaseModelPtr& model);

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
		
		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
		

        const std::string& getCommodityCurrencyPair() const
        {
            return m_commodityCurrencyPair;
        }

        double getConvexityAdjustment() const
		{
			return m_convexityAdjustment;
		}

    protected:
        CommodityFutures(const CommodityFutures& original, CloneLookup& lookup);
    
    private:

        static LT::date deliveryDate(const std::string& description, const ModuleDate::CalendarPtr& futuresCalendar, const std::string& EOMdeliveryOffset);

        double m_futurePrice;
        double m_forwardPrice;
        double m_convexityAdjustment;
        double m_deliveryTime;
        double m_contractSize;
        std::string m_commodityCurrencyPair;
        DiscountFactorPtr	m_deliveryDiscountFactor;
    }; 
    
    DECLARE_SMART_PTRS( CommodityFutures )

}
#endif //__LIBRARY_PRICERS_FLEXYCF_COMMODITYFUTURES_H_INCLUDED