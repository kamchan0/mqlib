#ifndef __LIBRARY_PRICERS_FLEXYCF_ZERORATE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_ZERORATE_H_INCLUDED
#pragma once

//	FlexYCF
#include "CalibrationInstruments.h"
#include "DiscountFactor.h"
#include "Gradient.h"

#include "lt\ptr.h"

namespace IDeA
{
    class AssetDomain;
}

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( PriceSupplier )
}

namespace FlexYCF
{
    class GlobalComponentCache;

    class ZeroRate : public CalibrationInstrument
    {
    public:
        ZeroRate(const std::string& description, const LT::date endDate, const double rate, const LT::date valueDate, GlobalComponentCache& globalComponentCache);
		
		ZeroRate(const std::string& tenorDescription, const LT::date endDate, const double rate, const LT::date valueDate);

        virtual ~ZeroRate();

        static std::string getName()
        {
            return "ZeroRate";
        }

		static CalibrationInstrumentPtr create(const LTQuant::GenericData& instrumentParametersTable, 
											   const LT::Ptr<LT::date>& buildDate,
											   const LTQuant::GenericData& curveParametersTable);

        static void createInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);

        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);

        double getTenor() const;
     
        virtual std::string getType() const
        {
            return ZeroRate::getName();
        }

        virtual const double getMarketPrice() const
        {
            return getRate();
        }

        virtual const double computeModelPrice(const BaseModelPtr) const;

        virtual const double computePV(const BaseModelPtr model) const;

        virtual const double computeBPV(const BaseModelPtr model) const;

        virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier, 
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd);
		virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);
		
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
        virtual void update();
        
        virtual void finishCalibration(const BaseModelPtr model);

        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr & src);

        virtual double getLastRelevantTime() const;
		
        virtual double getVariableInitialGuess(const double flowTime,
											   const BaseModel* const model) const;
		
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

		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);
		
        virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
        
        virtual std::ostream& print(std::ostream& out) const;    
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
        
    private:
        DiscountFactorPtr	m_discountFactor;                                 
    };

    DECLARE_SMART_PTRS( ZeroRate )
}
#endif