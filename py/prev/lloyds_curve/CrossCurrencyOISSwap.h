#ifndef __LIBRARY_PRICERS_FLEXYCF_CROSSCURRENCYOISSWAP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CROSSCURRENCYOISSWAP_H_INCLUDED
#pragma once

#include "CalibrationInstruments.h"

#include "ModuleDate/InternalInterface/ScheduleGenerator.h"
#include "AllComponentsAndCaches.h"
#include "Gradient.h"
#include "IDeA\src\market\MarketConvention.h"
#include "QCEnums.h"

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

    /// CrossCurrencySwap represents a cross-Currency swap that
    /// can be used to calibrate a model.
    class CrossCurrencyOISSwap : public CalibrationInstrument
    {
    public:
		CrossCurrencyOISSwap(const string& description,
                          const LT::date fixingDate,
                          const LT::date startDate,
                          const LT::date endDate,
                          const double spread,
						  const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
						  const LT::date valueDate,
						  const EndDateCalculationType backStubEndDateCalculationType);
		
        CrossCurrencyOISSwap(const string& description,
                          const LT::date fixingDate,
                          const LT::date startDate,
                          const LT::date endDate,
                          const double spread,
						  const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
                          const LT::date valueDate,
						  const EndDateCalculationType backStubEndDateCalculationType,
                          GlobalComponentCache& globalComponentCache);

        static std::string getName()
        {
            return "Ccy OIS Basis";
        }

		static CalibrationInstrumentPtr create(const LTQuant::GenericData& instrumentParametersTable, 
											   const LT::Ptr<LT::date>& buildDate,
											   const LTQuant::GenericData& curveParametersTable,
											   const LTQuant::GenericDataPtr&);

        static void createInstruments(CalibrationInstruments& instruments, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);
    
        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);

		static EndDateCalculationType::Enum_t defaultBackStubEndDataCalculationType()
		{
			return EndDateCalculationType::FromAccrualEndDate;
		}

        double getDomesticLegTenor() const
        {
            return m_domesticLegTenor;
        }

        virtual string getType() const
        {
            return CrossCurrencyOISSwap::getName();
        }

        virtual const double getMarketPrice() const
        {
            return 0.0;
        }

        /// Returns the Tenor of the floating leg
		inline std::string getDomesticFloatingLegTenorDesc() const	{	return m_details.m_floatFrequency.asTenorString().string();	}
		inline std::string getDomesticFloatBasis() const { return m_details.m_depRateMktConvention.m_dcm.asString().data(); }
		inline std::string getForeignFloatingLegTenorDesc() const	{	return m_details.m_forFloatFrequency.asTenorString().string();	}
		
		IDeA::LegType domesticLegType()  const   {   return m_details.m_domLegType; }

        virtual const double computeModelPrice(const BaseModelPtr) const;
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
							      IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
							      const double multiplier, 
							      IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);
		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
        virtual void finishCalibration(const BaseModelPtr model);
        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src);
    
        double getLastRelevantTime() const 
        {
            return ModuleDate::getYearsBetween(m_fixedLeg->getValueDate(), getEndDate()); 
        }
    

    protected:
        CrossCurrencyOISSwap(CrossCurrencyOISSwap const& original, CloneLookup& lookup);

    private:
		virtual void fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const;
		virtual void fillCashFlowPVsTable(const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable) const;
        CrossCurrencyOISSwap(CrossCurrencyOISSwap const&); // deliberately disabled as won't clone properly
        
        double fxRate(const BaseModelPtr& model) const
        {
            return m_fxSpot * m_fxSpotDomDiscountFactor->getValue(*model)/m_fxSpotForDiscountFactor->getValue(*m_foreignModel);
        }

        void initializeForeignLegPricing(const BaseModelPtr model) const;
     
         
        void initializeForeignLegPricing(const BaseModel& model) const;

		double									m_domesticLegTenor;
		IDeA::CurrencyBasisSwapMktConvention	m_details;
        FloatingLegPtr							m_floatingLeg;
        FixedLegPtr								m_fixedLeg; 
        DiscountFactorPtr						m_firstDiscountFactor;
        DiscountFactorPtr						m_lastDiscountFactor;
        
        // Foreign leg pricing
        mutable bool                            m_isInitialized;
        FloatingLegPtr							m_foreignLeg;
        mutable BaseModelConstPtr               m_foreignModel;
        mutable LT::Str                         m_foreignCcy;
        mutable double                          m_fxSpot;
        mutable DiscountFactorPtr				m_fxSpotDomDiscountFactor;
        mutable DiscountFactorPtr				m_fxSpotForDiscountFactor;
        DiscountFactorPtr						m_firstForeignDiscountFactor;
        DiscountFactorPtr						m_lastForeignDiscountFactor;

        Gradient m_tmpGradient;
    };  
    
    DECLARE_SMART_PTRS( CrossCurrencyOISSwap )

}
#endif 