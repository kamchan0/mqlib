/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FORWARDRATEAGREEMENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FORWARDRATEAGREEMENT_H_INCLUDED
#pragma once

#include "CalibrationInstruments.h"
#include "ForwardRate.h"
#include "Gradient.h"


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

    /// ForwardRateAgreement represents a forward rate agreement instrument that
    /// can be used to calibrate a model.
    /// 
    class ForwardRateAgreement : public CalibrationInstrument
    {
    public:
        ForwardRateAgreement(const std::string description,
                             const LT::date valueDate,
                             const LT::date fixingDate,
                             const LT::date startDate, 
                             const LT::date endDate,
                             const double quote,
                             const LTQC::Tenor rateTenor,
                             const ModuleDate::DayCounterConstPtr basis,
                             GlobalComponentCache& globalComponentCache);

        ForwardRateAgreement(const std::string description,
                             const LT::date valueDate,
                             const LT::date fixingDate,
                             const LT::date startDate, 
                             const LT::date endDate,
                             const double quote,
                             const LTQC::Tenor rateTenor,
                             const ModuleDate::DayCounterConstPtr basis);

        static std::string getName()
        {
            return getKeyName<ForwardRateAgreement>().data();
        }

		static CalibrationInstrumentPtr create(const LTQuant::GenericData& instrumentParametersTable,
											   const LT::Ptr<LT::date>& buildDate,
											   const LTQuant::GenericData& curveParametersTable,
											   const LTQuant::GenericDataPtr&);

        static void createInstruments(CalibrationInstruments& instruments, 
                                      LTQuant::GenericDataPtr instrumentTable,
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr priceSupplier);
    
        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);

        virtual CurveTypeConstPtr getCurveType() const;

        virtual std::string getType() const
        {
            return ForwardRateAgreement::getName();
        }

        /// returns the market quote
        virtual const double getMarketPrice() const;

        /// returns the model price
        virtual const double computeModelPrice(const BaseModelPtr) const;

        /// Computes the PV of the instrument
        virtual const double computePV(const BaseModelPtr model) const;

        /// Computes the basis point value of the instrument
        virtual const double computeBPV(const BaseModelPtr model) const;
        
        /// Accumulates the gradient of the corresponding implied LIBOR or the forward rate,
        /// depending on whether the Futures has a convexity model or not, resp.
        virtual void accumulateGradient(BaseModel const& baseModel, 
										double multiplier,
										GradientIterator gradientBegin,
										GradientIterator gradientEnd);
		virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);

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
		virtual double computeParRate(const BaseModelPtr& model);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
							      const double multiplier, 
							      IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);
        virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
							      const double multiplier, 
							      IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);
		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        ForwardRateAgreement(ForwardRateAgreement const& original, CloneLookup& lookup);

    private:
        ForwardRateAgreement(ForwardRateAgreement const&); // deliberately disabled as won't clone properly

        ForwardRatePtr			m_forwardRate;
        CurveTypeConstPtr		m_curveType;
    };  // ForwardRateAgreement

    inline CurveTypeConstPtr ForwardRateAgreement::getCurveType() const
    {
        return m_curveType;
    }

    DECLARE_SMART_PTRS( ForwardRateAgreement )

}   // FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FORWARDRATEAGREEMENT_H_INCLUDED