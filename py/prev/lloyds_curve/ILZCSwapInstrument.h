/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_ILZCSWAPINSTRUMENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_ILZCSWAPINSTRUMENT_H_INCLUDED
#pragma once

#include "CalibrationInstruments.h"
#include "InflationIndex.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( PriceSupplier )
}


namespace FlexYCF
{
	class GlobalComponentCache;
	FWD_DECLARE_SMART_PTRS( InflationModel )

 
	// Represents an Inflation-Linked Zero-Coupon Swap 
    class ILZCSwapInstrument : public CalibrationInstrument
    {
    public:
        ILZCSwapInstrument(	LT::date buildDate, 
							const std::string& maturity,  
							const std::string& mktConventionName, 
							double yield, 
							LTQuant::GenericDataPtr mktConventionTable, 
							LTQuant::GenericDataPtr assetTable, 
							size_t mktConventionEntry, 
							GlobalComponentCache& globalComponentCache);

        static std::string getName()
        {
            return "ILZCSwap";
        }

        static void createInstruments(CalibrationInstruments& instruments,
                                      LTQuant::GenericDataPtr instrumentTable,
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);
    
        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);

        virtual void finishCalibration(const BaseModelPtr model);

        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src);

        virtual std::string getType() const
        {
            return ILZCSwapInstrument::getName();
        }

        virtual const double getMarketPrice() const
        {
            return m_marketYield;
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
		

		// Returns the reset time lag from the build date in number of years
		double getResetTimeLag() const;

		double getForwardStartIndex(BaseModel const& inflationModel) const;
		double getForwardEndIndex(BaseModel const& inflationModel) const;

		double getRateCoefficient() const;

    private:
        double m_timeUnits;
        double m_tau;

        double m_marketYield;

		InflationIndexPtr m_indexStart;
		InflationIndexPtr m_indexEnd;
    };  //  ILZCSwapInstrument
    
    DECLARE_SMART_PTRS( ILZCSwapInstrument )

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_ILZCSWAPINSTRUMENT_H_INCLUDED