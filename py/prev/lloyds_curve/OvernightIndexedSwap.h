
#ifndef __LIBRARY_PRICERS_FLEXYCF_OVERNIGHTINDEXEDSWAP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_OVERNIGHTINDEXEDSWAP_H_INCLUDED
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
    FWD_DECLARE_SMART_PTRS( DayCounter )
    FWD_DECLARE_SMART_PTRS( PriceSupplier )
}

namespace FlexYCF
{
    class GlobalComponentCache;

    class OvernightIndexedSwap : public CalibrationInstrument
    {
    public:
		OvernightIndexedSwap(const string& description,
                         const LT::date fixingDate,
                         const LT::date startDate,
						 const LT::date endDate,
                         const LT::date lastDate,
					     const double swapRate,
						 const IDeA::OISSwapMktConvention& tradeDetails,
					     const LT::date valueDate,
						 GlobalComponentCache& globalComponentCache);

		OvernightIndexedSwap(const string& description,
                         const LT::date fixingDate,
                         const LT::date startDate,
						 const LT::date endDate,
                         const LT::date lastDate,
					     const double swapRate,
						 const IDeA::OISSwapMktConvention& tradeDetails,
					     const LT::date valueDate);

      
        static std::string getName()
        {
            return "OIS";
        }

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
		inline std::string getFloatingLegTenorDesc() const	{	return m_details.m_oisMktConvention.m_floatFrequency.asTenorString().string();	}

		inline std::string getFixedBasis() const { return m_details.m_fixedAccrualBasis.asString().data(); }
		inline std::string getRatesBasis() const { return m_details.m_oisMktConvention.m_depRateMktConvention.m_dcm.asString().data(); }
        inline std::string getFloatBasis() const { return m_details.m_oisMktConvention.m_floatAccrualBasis.asString().data(); }

		inline std::string getAccrualCalendar() const { return m_details.m_fixedAccrualBasis.asString().data(); }
		inline std::string getSpotDays() const { return m_details.m_oisMktConvention.m_spotDays.asTenorString().string(); }
        inline std::string getPayDelay() const { return m_details.m_oisMktConvention.m_payDelay.asTenorString().string(); }
		inline std::string getPayCalendar() const { return m_details.m_oisMktConvention.m_payCalendar.string(); }
        inline std::string getIndex() const { return m_details.m_oisMktConvention.m_index.string(); }
        
        virtual string getType() const
        {
            return OvernightIndexedSwap::getName();
        }

        virtual const double getMarketPrice() const
        {
            return 0.0;
        }
        
        virtual double getLastRelevantTime() const;

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
		
        virtual void finishCalibration(const BaseModelPtr model);

        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src);

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
        OvernightIndexedSwap(OvernightIndexedSwap const& original, CloneLookup& lookup);

    private:
		virtual void fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const;
		virtual void fillCashFlowPVsTable(const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable) const;
        OvernightIndexedSwap(OvernightIndexedSwap const&); // deliberately disabled as won't clone properly

		IDeA::OISSwapMktConvention	m_details;

        double m_fixedLegTenor;
        double m_floatingLegTenor;
         
        FixedLegPtr     m_fixedLeg;
        FloatingLegPtr  m_floatingLeg;
    }; 
    
    DECLARE_SMART_PTRS( OvernightIndexedSwap )
} 
#endif //__LIBRARY_PRICERS_FLEXYCF_OVERNIGHTINDEXEDSWAP_H_INCLUDED