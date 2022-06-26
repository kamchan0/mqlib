/*****************************************************************************

    Cash Instrument

	Represents a cash instrument.


    @Originator		Nicolas Maury
	   
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CASHINSTRUMENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CASHINSTRUMENT_H_INCLUDED
#pragma once

//	FlexYCF
#include "CalibrationInstruments.h"
#include "ForwardRate.h"
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

namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
}

namespace FlexYCF
{
    class GlobalComponentCache;

    /// CashInstrument represents a cash instrument that
    /// can be used to calibrate a model.
    class CashInstrument : public CalibrationInstrument
    {
    public:
        CashInstrument(const std::string& description, 
                       const LT::date fixingDate,
                       const LT::date startDate,
                       const LT::date endDate,
                       const double rate,
                       const ModuleDate::DayCounterConstPtr basis,
                       const LT::date valueDate,
                       GlobalComponentCache& globalComponentCache);
		
		CashInstrument(const std::string& tenorDescription,
					   const LT::date fixingDate,
                       const LT::date startDate,
                       const LT::date endDate,
                       const double rate,
                       const ModuleDate::DayCounterConstPtr basis,
                       const LT::date valueDate);

        virtual ~CashInstrument();

        static std::string getName()
        {
            return "Cash";
        }

		static CalibrationInstrumentPtr create(const LTQuant::GenericData& instrumentParametersTable, 
											   const LT::Ptr<LT::date>& buildDate,
											   const LTQuant::GenericData& curveParametersTable,
											   const LTQuant::GenericDataPtr&);

        static void createInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);

        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);
        
        /// Returns the Tenor of the cash instrument
        double getTenor() const;
        double getCoverage() const;
        
        ModuleDate::DayCounterConstPtr basis() const
        {
            return  m_forwardRate->getArguments().basis();    
        }
        
        virtual std::string getType() const
        {
            return CashInstrument::getName();
        }

        virtual const double getMarketPrice() const
        {
            return getRate();
        }

        virtual const double computeModelPrice(const BaseModelPtr) const;

        /// Computes the PV of the instrument
        virtual const double computePV(const BaseModelPtr model) const;

        /// Computes the basis point value of the instrument
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

		//	IHasRepFlows<IDeA::Index> interface
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);

		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
        virtual std::ostream& print(std::ostream& out) const;     // Useful for testing

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
        
    private:
		//explicit CashInstrument(const date fixingDate, const date startDate,
		//						const date endDate);

        ForwardRatePtr  m_forwardRate;          // [1 + cvg(t,T) * F(0; t, T)] in the formula of the cash instrument
                                                //  can also be represented as TDF(0, t) / TDF(0, T), with the appropriate Tenor
    };

    DECLARE_SMART_PTRS( CashInstrument )

}
#endif //__LIBRARY_PRICERS_FLEXYCF_CASHINSTRUMENT_H_INCLUDED