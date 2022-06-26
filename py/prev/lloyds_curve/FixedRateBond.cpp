#include "stdafx.h"

#include "BaseModel.h"
#include "Data/GenericData.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"
#include "AllComponentsAndCaches.h"
#include "GlobalComponentCache.h"
#include "TenorUtils.h"
#include "DataExtraction.h"
#include "FlexYcfUtils.h"
#include "FlexYCFCloneLookup.h"
#include "BondTradeFactory.h"
#include "FixedRateBond.h"
#include "ScheduleUtils.h"

// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "MarketConvention.h"

// std
#include <functional>
#include <algorithm>

using namespace LTQC;
using namespace IDeA;
using namespace std;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<FixedRateBondInstrument>()
	{
		return IDeA_KEY(BONDINSTRUMENTTYPE, FIXEDRATEBONDINSTRUMENT);
	}

	FixedRateBondInstrument::FixedRateBondInstrument(	const double rate,
									const LT::Str& description,
									const LT::date fixingDate,
									const LT::date startDate,
									const LT::date endDate,
									const SettledBondPtr& setBond,
									const QuoteType& quoteType)
		: CalibrationInstrument(rate, getKeyName<FixedRateBondInstrument>(), description, fixingDate, startDate, endDate, setBond->m_xbond->getIdentifier())
		, m_setBond(setBond), m_quoteType(quoteType)
		, m_fixedLeg(FixedLeg::create
			(
				FixedLeg::Arguments(setBond->m_contractDate.getAsLTdate(), 
				startDate, 
				startDate, 
				endDate, 
				setBond->m_xbond->m_bond->m_bondDefinition->m_frequency.string(), 
				// The following basis is arbitrarily set, since ActActISMA is not implemented in QDDate and therefore could not retrieved from setBond.
				// However it doesn't matter since m_basis is not used for bond instruments
				"Act/365",
				setBond->m_xbond->m_bond->m_bondDefinition->m_accrualCalendar.string(),
				setBond->getCurrency(),
				"",
				RollConvMethod(setBond->m_xbond->m_bond->m_bondDefinition->m_accrualRollConvention.asString()),
				setBond->m_xbond->m_bond->m_bondDefinition->m_accrualDurationModifyConvention,
				"0B",
				setBond->m_xbond->m_bond->m_bondDefinition->m_calendar.string(),
				setBond->m_xbond->m_bond->m_bondDefinition->m_rollConvention.asString(),
				setBond->m_xbond->m_bond->m_bondDefinition->m_stubType)
			))
		, m_notional(setBond->m_xbond->getNotional())
		, m_coupon(setBond->m_xbond->m_bond->m_bondDefinition->m_coupon)
		, m_endDateTime(ModuleDate::getYearsBetween(startDate, endDate))
		, m_settleToTradeTime(ModuleDate::getYearsBetween(startDate, m_setBond->getSettlementDate().getAsLTdate()))
	{
		setFixedLegCvgPayDates(m_fixedLeg, m_setBond);
	}

	FixedRateBondInstrument::FixedRateBondInstrument(	const double rate, 
									const LT::Str& description, 
									const LT::date fixingDate, 
									const LT::date startDate, 
									const LT::date endDate, 
									const SettledBondPtr& setBond, 
									const QuoteType& quoteType,
									GlobalComponentCache& globalComponentCache )
		: CalibrationInstrument(rate, getKeyName<FixedRateBondInstrument>(), description, fixingDate, startDate, endDate, setBond->m_xbond->getIdentifier())
		, m_setBond(setBond), m_quoteType(quoteType)
		, m_fixedLeg(globalComponentCache.get
			(
				FixedLeg::Arguments(setBond->m_contractDate.getAsLTdate(), 
				startDate, 
				startDate, 
				endDate, 
				setBond->m_xbond->m_bond->m_bondDefinition->m_frequency.string(),
				// The following basis is arbitrarily set, since ActActISMA is not implemented in QDDate and therefore could not retrieved from setBond.
				// However it doesn't matter since m_basis is not used for bond instruments
				"Act/365",	
				setBond->m_xbond->m_bond->m_bondDefinition->m_accrualCalendar.string(),
				setBond->getCurrency(),
				"",
				RollConvMethod(setBond->m_xbond->m_bond->m_bondDefinition->m_accrualRollConvention.asString()),
				setBond->m_xbond->m_bond->m_bondDefinition->m_accrualDurationModifyConvention,
				"0B",
				setBond->m_xbond->m_bond->m_bondDefinition->m_calendar.string(),
				setBond->m_xbond->m_bond->m_bondDefinition->m_rollConvention.asString(),
				setBond->m_xbond->m_bond->m_bondDefinition->m_stubType)
			))
		, m_notional(setBond->m_xbond->getNotional())
		, m_coupon(setBond->m_xbond->m_bond->m_bondDefinition->m_coupon)
		, m_endDateTime(ModuleDate::getYearsBetween(startDate, endDate))
		, m_settleToTradeTime(ModuleDate::getYearsBetween(startDate, m_setBond->getSettlementDate().getAsLTdate()))
	{
		setFixedLegCvgPayDates(m_fixedLeg, m_setBond);
	}

	FixedRateBondInstrument::FixedRateBondInstrument( FixedRateBondInstrument const& original, CloneLookup& lookup )
		:	CalibrationInstrument(original),
			m_quoteType(original.m_quoteType),
			m_setBond(original.m_setBond),
			m_settleToTradeTime(original.m_settleToTradeTime),
			m_fixedLeg(lookup.get(original.m_fixedLeg))	 
	{
	}
	
	void FixedRateBondInstrument::createInstruments(	CalibrationInstruments& instruments,
											LTQuant::GenericDataPtr instrumentTable,
											LTQuant::GenericDataPtr masterTable,
											GlobalComponentCache& globalComponentCache,
											const LTQuant::PriceSupplierPtr)
	{
		// just finish if we have empty table or just headings
		if(instrumentTable->numItems() < 2)
		{
			return;
		}

		const std::vector<SettledBondPtr> setBondVec = createSettledBondVec(instrumentTable, masterTable, BondType::FixedRateBond);
		
		for(size_t i = 0; i < setBondVec.size(); ++i)
		{
			// mandatory input for each bond instrument
			const double quote = IDeA::extract<double>(instrumentTable->table, IDeA_KEY(BONDINSTRUMENT, QUOTE), i);
			
			// bond quote type is optional, missing field or empty value interpreted as default quote type CleanPrice
			const QuoteType defaultType(QuoteType::CleanPrice);
			QuoteType quoteType;
			LT::Str defaultTypeStr(defaultType.asString()), quoteTypeStr;
			permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(BONDINSTRUMENT, QUOTETYPE), i, quoteTypeStr, defaultTypeStr);
			if (quoteTypeStr.empty())
				quoteType = defaultType;
			else
				quoteType = QuoteType(quoteTypeStr);

			// !endDate is not bond maturity date (which could be on holiday), it should be the bond last payment date
			LT::date endDate = setBondVec[i]->m_xbond->getLastPaymentDate().getAsLTdate();
			LT::date valueDate = setBondVec[i]->m_contractDate;

			CalibrationInstrumentPtr instrument( new FixedRateBondInstrument(quote, endDate.toDDMMMYYYYString(), valueDate, valueDate, endDate, setBondVec[i], quoteType, globalComponentCache));
			instruments.add(instrument);
		}
	}

	CalibrationInstrumentPtr FixedRateBondInstrument::create( const LTQuant::GenericData& instrumentParametersTable, 
													const LT::Ptr<LT::date>& buildDate, 
													const LTQuant::GenericData& curveParametersTable,
													const LTQuant::GenericDataPtr& extraInfoTable)
	{
		// QuoteType: optional, missing field or empty value interpreted as default quote type CleanPrice
		const QuoteType defaultType(QuoteType::CleanPrice);
		QuoteType quoteType;
		LT::Str defaultTypeStr(defaultType.asString()), quoteTypeStr;
		permissive_extract<LT::Str>(instrumentParametersTable.table, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, QUOTETYPE), quoteTypeStr, defaultTypeStr);
		if (quoteTypeStr.empty())
			quoteType = defaultType;
		else
			quoteType = QuoteType(quoteTypeStr);

		SettledBondPtr setBond = createSetBondForInstrument(instrumentParametersTable.table, curveParametersTable, *buildDate, extraInfoTable, BondType::FixedRateBond);

		LT::date endDate = setBond->m_xbond->getLastPaymentDate().getAsLTdate();

		return CalibrationInstrumentPtr(new FixedRateBondInstrument(1.0, endDate.toDDMMMYYYYString(), *buildDate, *buildDate, endDate, setBond, quoteType));
	}

	const double FixedRateBondInstrument::getMarketPrice() const
	{
		return m_setBond->getPerBondNominalDirtyPrice(getRate(), m_quoteType);
	}

	const double FixedRateBondInstrument::computeModelPrice( const BaseModelPtr model) const
	{
		const double couponLegValue = m_fixedLeg->getValue(*model) * m_coupon * m_notional;
		const double redemptionValue = m_notional * model->getDiscountFactor(m_endDateTime);

		// revert back the valueDate pv to settlementDate price:
		return (couponLegValue + redemptionValue) / model->getDiscountFactor(m_settleToTradeTime);
	}

	double FixedRateBondInstrument::computePV( const BaseModelPtr model )
	{
		return getResidual(model) * model->getDiscountFactor(m_settleToTradeTime);
	}

	double FixedRateBondInstrument::computeBPV( const BaseModelPtr model )
	{
		return oneBasisPoint() * model->getDiscountFactor(m_settleToTradeTime) * calculateRateDerivative(model);
	}

	void FixedRateBondInstrument::accumulateGradient( BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd )
	{
		const double couponLegValue = m_fixedLeg->getValue(baseModel) * m_coupon * m_notional;
		const double redemptionValue = m_notional * baseModel.getDiscountFactor(m_endDateTime);
		const double modelPrice = (couponLegValue + redemptionValue) / baseModel.getDiscountFactor(m_settleToTradeTime);

		const double inverseDf = 1.0 / baseModel.getDiscountFactor(m_settleToTradeTime);
		const double mulDfSettle = - inverseDf * modelPrice * multiplier;
		const double mulDfEndDate = m_notional * inverseDf * multiplier;
		const double mulCouponLeg = m_coupon * mulDfEndDate;

		baseModel.accumulateDiscountFactorGradient(m_settleToTradeTime, mulDfSettle, gradientBegin, gradientEnd);
		baseModel.accumulateDiscountFactorGradient(m_endDateTime, mulDfEndDate, gradientBegin, gradientEnd);
		m_fixedLeg->accumulateGradient(baseModel, mulCouponLeg, gradientBegin, gradientEnd);
	}

	void FixedRateBondInstrument::accumulateGradient( BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, const CurveTypeConstPtr& curveType )
	{
		const double couponLegValue = m_fixedLeg->getValue(baseModel) * m_coupon * m_notional;
		const double redemptionValue = m_notional * baseModel.getDiscountFactor(m_endDateTime);
		const double modelPrice = (couponLegValue + redemptionValue) / baseModel.getDiscountFactor(m_settleToTradeTime);

		const double inverseDf = 1.0 / baseModel.getDiscountFactor(m_settleToTradeTime);
		const double mulDfSettle = - inverseDf * modelPrice * multiplier;
		const double mulDfEndDate = m_notional * inverseDf * multiplier;
		const double mulCouponLeg = m_coupon * mulDfEndDate;

		baseModel.accumulateDiscountFactorGradient(m_settleToTradeTime, mulDfSettle, gradientBegin, gradientEnd, curveType);
		baseModel.accumulateDiscountFactorGradient(m_endDateTime, mulDfEndDate, gradientBegin, gradientEnd, curveType);
		m_fixedLeg->accumulateGradient(baseModel, mulCouponLeg, gradientBegin, gradientEnd, curveType);
	}
	
	void FixedRateBondInstrument::accumulateGradientConstantDiscountFactor(
								BaseModel const& baseModel, BaseModel const& dfModel, 
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

	void FixedRateBondInstrument::accumulateGradientConstantTenorDiscountFactor(
								BaseModel const& baseModel, BaseModel const& dfModel, 
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

	double FixedRateBondInstrument::calculateRateDerivative( const BaseModelPtr& model ) const
	{
		// TODO: temporary solution:
		const double shift = 1E-10;
		return (  m_setBond->getPerBondNominalDirtyPrice(getRate() + shift, m_quoteType) 
				- m_setBond->getPerBondNominalDirtyPrice(getRate()- shift, m_quoteType)
				) / (2.0 * shift);
	}

	void FixedRateBondInstrument::accumulateRateDerivativeGradient( const BaseModel& model, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd ) const
	{
		// do nothing
	}

	void FixedRateBondInstrument::accumulateRateDerivativeGradient( const BaseModel& model, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, const CurveTypeConstPtr& curveType ) const
	{
		// do nothing
	}

	FlexYCF::ICloneLookupPtr FixedRateBondInstrument::cloneWithLookup( CloneLookup& lookup ) const
	{
		return CalibrationInstrumentPtr(new FixedRateBondInstrument(*this, lookup));
	}

	void FixedRateBondInstrument::finishCalibration( const BaseModelPtr model )
	{
		CalibrationInstrument::finishCalibration(model);
		m_fixedLeg->cleanupCashFlows();
	}

	void FixedRateBondInstrument::reloadInternalState( const CalibrationInstrumentPtr& src )
	{ 
		const FixedRateBondInstrumentPtr ourTypeSrc=std::tr1::static_pointer_cast<FixedRateBondInstrument>(src);
		if(!ourTypeSrc)
		{
			LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
		}

		CalibrationInstrument::reloadInternalState(src);
	}

	void FixedRateBondInstrument::update()
	{
		m_fixedLeg->update();
	}

	double FixedRateBondInstrument::getDifferenceWithNewRate( const LTQuant::GenericData& instrumentListData ) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<FixedRateBondInstrument>(), IDeA_KEY(BONDINSTRUMENT, ISIN), IDeA_KEY(BONDINSTRUMENT, QUOTE));
	}

	double FixedRateBondInstrument::computeParRate( const BaseModelPtr& model )
	{
		double nominalDirtyPrice = computeModelPrice(model);
		return m_setBond->getQuoteFromPerBondNominalDirtyPrice(nominalDirtyPrice, m_quoteType);
	}

	void FixedRateBondInstrument::updateInstruments( CalibrationInstruments& instrumentList, LTQuant::GenericDataPtr instrumentTable, size_t* instrumentIndex )
	{
		const size_t nbBond(IDeA::numberOfRecords(*instrumentTable));

		// just finish if we have empty table or just headings
		if(nbBond == 0)
		{
			return;
		}

		for(size_t i = 0; i < nbBond; ++i)
		{
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(BONDINSTRUMENT, ISIN), i));
			if(!description.empty())
			{
				const double bondQuote(IDeA::extract<double>(*instrumentTable, IDeA_KEY(BONDINSTRUMENT, QUOTE), i));

				instrumentList[*instrumentIndex]->setRate(bondQuote);
				++(*instrumentIndex);
			}
		}

	}

	double FixedRateBondInstrument::getLastRelevantTime() const
	{
		return m_endDateTime;
	}

	void FixedRateBondInstrument::fillCashFlowsTable( LTQuant::GenericData& cashFlowsTable ) const
	{
		// create a redemption flow schedule
		std::string redemptionLeg = "Redemption Leg";
		const LTQuant::GenericDataPtr redemptionFlow(new LTQuant::GenericData(redemptionLeg, 0));
		redemptionFlow->set<LT::date>("Settlement Date", 0, m_setBond->getSettlementDate().getAsLTdate());
		redemptionFlow->set<LT::date>("Maturity Date", 0, m_setBond->m_xbond->getMaturityDate().getAsLTdate());

		// fill cash flow from fixLeg and redemption flow
		cashFlowsTable.set<LTQuant::GenericDataPtr>("Coupon Leg", 0, m_fixedLeg->getCashFlows());
		cashFlowsTable.set<LTQuant::GenericDataPtr>(redemptionLeg, 0, redemptionFlow);

	}

	void FixedRateBondInstrument::fillCashFlowPVsTable( const BaseModel& model, LTQuant::GenericData& cashFlowPVsTable ) const
	{
		// create a redemption leg pv flow
		std::string redemptionLeg = "Redemption Leg";
		const LTQuant::GenericDataPtr redemptionFlowPvTbl(new LTQuant::GenericData(redemptionLeg, 0));
		redemptionFlowPvTbl->set<LT::date>("Settlement Date", 0, m_setBond->getSettlementDate().getAsLTdate());
		redemptionFlowPvTbl->set<LT::date>("Maturity Date", 0, m_setBond->m_xbond->getMaturityDate().getAsLTdate());
		redemptionFlowPvTbl->set<double>("Discount Factor", 0, model.getDiscountFactor(m_endDateTime));
		redemptionFlowPvTbl->set<LT::date>("Payment Date", 0, m_setBond->m_xbond->getLastPaymentDate().getAsLTdate());

		// fill cash flow pv table from fixLeg and redemption flow
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Coupon Leg", 0, m_fixedLeg->computeCashFlowPVs(model));
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>(redemptionLeg, 0, redemptionFlowPvTbl);
	}

}
