/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INTERESTRATESWAP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INTERESTRATESWAP_H_INCLUDED
#pragma once

#include "CalibrationInstruments.h"
#include "ModuleDate/InternalInterface/ScheduleGenerator.h"
#include "AllComponentsAndCaches.h"
#include "Gradient.h"
#include "IDeA\src\market\MarketConvention.h"

namespace IDeA
{
    class AssetDomain;
}

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( PriceSupplier )
}

namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
}

namespace FlexYCF
{
    class GlobalComponentCache;

    /// InterestRateSwap represents an Interest Rate Swap
    /// instrument that can be used to calibrate a model.
    class InterestRateSwap : public CalibrationInstrument
    {
    public:
		InterestRateSwap(const string& description,
                         const LT::date fixingDate,
                         const LT::date startDate,
						 const LT::date endDate,
					     const double swapRate,
						 const IDeA::SwapMktConvention& tradeDetails,
					     const LT::date valueDate,
						 GlobalComponentCache& globalComponentCache);

		InterestRateSwap(const string& description,
                         const LT::date fixingDate,
                         const LT::date startDate,
						 const LT::date endDate,
					     const double swapRate,
						 const IDeA::SwapMktConvention& tradeDetails,
					     const LT::date valueDate);

        // This constructor handles LIBOR fixing
		InterestRateSwap(const string& description,
                         const LT::date fixingDate,
                         const LT::date startDate,
						 const LT::date endDate,
					     const double swapRate,
						 const IDeA::SwapMktConvention& tradeDetails,
					     const LT::date valueDate,
                         const double liborFixing,
						 GlobalComponentCache& globalComponentCache);

        static std::string getName()
        {
            return "Swaps";
        }

        virtual void finishCalibration(const BaseModelPtr model);

        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr & src);

		static CalibrationInstrumentPtr create(const LTQuant::GenericData& instrumentParametersTable, 
											   const LT::Ptr<LT::date>& buildDate,
											   const LTQuant::GenericData& curveParametersTable,
											   const LTQuant::GenericDataPtr&);

        /// Creates the interest rate swaps
        static void createInstruments(CalibrationInstruments& instruments,
                                      LTQuant::GenericDataPtr instrumentTable,
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);

        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);

        /// Returns the Tenor of the fixed leg
        inline double getFixedLegTenor() const			{	return m_fixedLegTenor;		}
		inline std::string getFixedLegTenorDesc() const {	return m_details.m_fixedFrequency.asTenorString().string(); }

        /// Returns the Tenor of the floating leg
        inline double getFloatingLegTenor() const			{	return m_floatingLegTenor;		}
		inline std::string getFloatingLegTenorDesc() const	{	return m_details.m_floatFrequency.asTenorString().string();	}

		inline std::string getFixedBasis() const { return m_details.m_fixedAccrualBasis.asString().data(); }
		inline std::string getFloatBasis() const { return m_details.m_depRateMktConvention.m_dcm.asString().data(); }

		
		virtual string getType() const
        {
            return InterestRateSwap::getName();
        }

        virtual const double getMarketPrice() const
        {
            return 0.0;
        }
        
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
							      IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
							      const double multiplier, 
							      IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);
		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
		virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        InterestRateSwap(InterestRateSwap const& original, CloneLookup& lookup);

    private:
		virtual void fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const;
		virtual void fillCashFlowPVsTable(const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable) const;
        InterestRateSwap(InterestRateSwap const&); // deliberately disabled as won't clone properly

		IDeA::SwapMktConvention	m_details;

		//std::string m_fixedLegTenorDesc;
		//std::string m_floatingLegTenorDesc;
		//std::string m_fixedBasis;
        double m_fixedLegTenor;
        double m_floatingLegTenor;
         
        FixedLegPtr     m_fixedLeg;
        FloatingLegPtr  m_floatingLeg;
    };  //  InterestRateSwap
    
    DECLARE_SMART_PTRS( InterestRateSwap )

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_INTERESTRATESWAP_H_INCLUDED