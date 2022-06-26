/*****************************************************************************
    Todo: - Add header file description


    @Originator

    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved
*****************************************************************************/

#ifndef __LIBRARY_PRICERS_FLEXYCF_INFLATIONBOND_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INFLATIONBOND_H_INCLUDED

// FlexYCF
#include "CalibrationInstrument.h"
#include "FixedLegAdjusted.h"

// IDeA
#include "SettledBond.h"

namespace FlexYCF
{
	class InflationIndexArguments;
	typedef FixedLegAdjusted<InflationIndexArguments> FixedLegILAdjusted;
	DECLARE_SMART_PTRS( FixedLegILAdjusted );

	class InflationBondInstrument : public CalibrationInstrument
	{
	public:
		InflationBondInstrument(	const double rate,
						const LT::Str& description,
						const LT::date& startDate,
						const LT::date& endDate,
						const IDeA::QuoteType& quoteType,
						const IDeA::SettledBondPtr& setBond,
						const InflationIndexPtr& redemptionILIndex,
						const FixedLegILAdjustedPtr& fxLegAdj);

		~InflationBondInstrument() {};

		static void createInstruments(	CalibrationInstruments& instruments,
										LTQuant::GenericDataPtr instrumentTable,
										LTQuant::GenericDataPtr masterTable,
										GlobalComponentCache& globalComponentCache,
										const LTQuant::PriceSupplierPtr);

		static CalibrationInstrumentPtr create(	const LTQuant::GenericData& instrumentParametersTable, 
												const LT::Ptr<LT::date>& buildDate,
												const LTQuant::GenericData& curveParametersTable,
												const LTQuant::GenericDataPtr& extraInfoTable);
		
		static void updateInstruments(	CalibrationInstruments& instrumentList, 
										LTQuant::GenericDataPtr instrumentTable, 
										size_t* instrumentIndex);

		static std::string getName()
		{
			return "InflationBondInstrument";
		}

		virtual void finishCalibration(const BaseModelPtr model);

		virtual void reloadInternalState(const CalibrationInstrumentPtr& src);

		virtual std::string getType() const
		{
			return getName();
		}

		// return the bond nominal dirty price
		virtual const double getMarketPrice() const;

		virtual const double computeModelPrice(const BaseModelPtr model) const;

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

		virtual double calculateRateDerivative(const BaseModelPtr& model) const;

		virtual void accumulateRateDerivativeGradient(	const BaseModel& model,
														double multiplier,
														GradientIterator gradientBegin,
														GradientIterator gradientEnd) const;

		virtual void accumulateRateDerivativeGradient(	const BaseModel& model,
														double multiplier,
														GradientIterator gradientBegin,
														GradientIterator gradientEnd,
														const CurveTypeConstPtr& curveType) const;
		 
		virtual double computeParRate(const BaseModelPtr& model);

		virtual double computePV(const BaseModelPtr model);

		virtual double computeBPV(const BaseModelPtr model);

		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;

		virtual double getLastRelevantTime() const;

		virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

	protected:
		InflationBondInstrument(InflationBondInstrument const& original, CloneLookup& lookup);

	private:
		FixedLegILAdjustedPtr	    m_fixedLegAdj;
		InflationIndexPtr		    m_redemptionILIndex;
		IDeA::SettledBondPtr	    m_setBond;
		mutable	BaseModelConstPtr	m_dependentModel;
		
		IDeA::QuoteType		        m_quoteType;
		double			            m_settleToTradeTime;
		double			            m_endDateTime;
		double			            m_notional;
		double			            m_coupon;
		double			            m_baseIndexValue;

		InflationBondInstrument(InflationBondInstrument const&); // disabled the copy constructor
		void initialize(const BaseModel& baseModel) const;
		virtual void fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const;
		virtual void fillCashFlowPVsTable(const BaseModel& model, LTQuant::GenericData& cashFlowPVsTable) const;
	};
		
	DECLARE_SMART_PTRS( InflationBondInstrument )
}

#endif