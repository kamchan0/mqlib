#include "stdafx.h"

//FlexYCF
#include "BaseModel.h"
#include "GlobalComponentCache.h"
#include "InflationBond.h"
#include "InflationIndex.h"
#include "ScheduleUtils.h"

// IDeA
#include "DictYieldCurve.h"

using namespace LTQC;
using namespace IDeA;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<InflationBondInstrument>()
	{
		return IDeA_KEY(BONDINSTRUMENTTYPE, INFLATIONBONDINSTRUMENT);
	}

	InflationBondInstrument::InflationBondInstrument(	const double rate,
									const LT::Str& description,
									const LT::date& startDate,
									const LT::date& endDate,
									const QuoteType& quoteType,
									const SettledBondPtr& setBond,
									const InflationIndexPtr& redemptionILIndex,
									const FixedLegILAdjustedPtr& fxLegAdj)
		: CalibrationInstrument(rate, getKeyName<InflationBondInstrument>(), description, startDate, startDate, endDate, setBond->m_xbond->getIdentifier())
		, m_setBond(setBond)
		, m_quoteType(quoteType)
		, m_redemptionILIndex(redemptionILIndex)
		, m_fixedLegAdj(fxLegAdj)
		, m_notional(setBond->m_xbond->getNotional())
		, m_coupon(setBond->m_xbond->getUnadjustedCoupon())
		, m_baseIndexValue(setBond->m_xbond->getBaseReferenceIndex())
		, m_endDateTime(ModuleDate::getYearsBetween(startDate, endDate))
		, m_settleToTradeTime(ModuleDate::getYearsBetween(startDate, m_setBond->getSettlementDate().getAsLTdate()))
	{
		setFixedLegCvgPayDates(m_fixedLegAdj->getFixedLeg(), m_setBond);
	}

	InflationBondInstrument::InflationBondInstrument( InflationBondInstrument const& original, CloneLookup& lookup )
		:	CalibrationInstrument(original)
		, m_setBond(original.m_setBond)
		, m_quoteType(original.m_quoteType)
		, m_redemptionILIndex(lookup.get(original.m_redemptionILIndex))
		, m_fixedLegAdj(lookup.get(original.m_fixedLegAdj))
		, m_notional(original.m_notional)
		, m_coupon(original.m_coupon)
		, m_baseIndexValue(original.m_baseIndexValue)
		, m_endDateTime(original.m_endDateTime)
		, m_settleToTradeTime(original.m_settleToTradeTime) 
	{
	}
	
	void InflationBondInstrument::createInstruments(	CalibrationInstruments& instruments,
											LTQuant::GenericDataPtr instrumentTable,
											LTQuant::GenericDataPtr masterTable,
											GlobalComponentCache& globalComponentCache,
											const LTQuant::PriceSupplierPtr)
	{
		using namespace std;
		using namespace std::placeholders;
		
		// just finish if we have empty table or just headings
		if(instrumentTable->numItems() < 2)
		{
			return;
		}

		const std::vector<SettledBondPtr> setBondVec = createSettledBondVec(instrumentTable, masterTable, BondType::InflationBond);

		for(size_t i = 0; i < setBondVec.size(); ++i)
		{
			const SettledBondPtr& setBond = setBondVec[i];

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
			LT::date endDate = setBond->m_xbond->getLastPaymentDate().getAsLTdate();
			LT::date startDate = setBond->m_contractDate;

			std::string timeLag, interp;
			try
			{
				const IDeA::ILRate& ilRate = dynamic_cast<const IDeA::ILRate&>(*setBond->m_xbond->m_bond->m_bondDefinition->m_rate);
				timeLag = ilRate.m_refIndex1->m_timeLag.asString().string();
				if(timeLag[0] != '-')
				{
					timeLag = "-" + timeLag;
				}
				interp	= ilRate.m_refIndex1->m_resetType.asString().data();
			}
			catch (const std::bad_cast& e)
			{
				LTQC_THROW( IDeA::ModelException, e.what() << ": InflationBondInstrument::createInstruments: the instrument is not an inflation bond." );
			}

			tr1::function<const ILIndexArg(const LT::date&, const LT::date&)> func = std::tr1::bind(getInflationIndexArguments, _1, _2, timeLag, interp);			
			FixedLegILAdjustedPtr fixLegAdj(new FixedLegAdjusted<InflationIndexArguments>(createFixedLegArgFromSetBond(setBond, globalComponentCache),func));

			// create redemption IL index
			const ILIndexArg ilArg = func(startDate, endDate);
			InflationIndexPtr redepmtionILIndex
				(InstrumentComponent::getUseCacheFlag()
									? globalComponentCache.get
											(InflationIndex::Arguments(	ilArg.forward1Time, 
																		ilArg.forward2Time,
																		ilArg.weight, 
																		endDate))
									: InflationIndex::create
											(InflationIndexArguments(	ilArg.forward1Time, 
																		ilArg.forward2Time,
																		ilArg.weight, 
																		endDate)));

			CalibrationInstrumentPtr instrument( new InflationBondInstrument(quote, endDate.toDDMMMYYYYString(), startDate, endDate, quoteType, setBond, redepmtionILIndex, fixLegAdj));
			instruments.add(instrument);
		}
	}

	CalibrationInstrumentPtr InflationBondInstrument::create( const LTQuant::GenericData& instrumentParametersTable, 
													const LT::Ptr<LT::date>& buildDate, 
													const LTQuant::GenericData& curveParametersTable, 
													const LTQuant::GenericDataPtr& extraInfoTable )
	{
		using namespace std;
		using namespace std::placeholders;

		// QuoteType: optional, missing field or empty value interpreted as default quote type CleanPrice
		const QuoteType defaultType(QuoteType::CleanPrice);
		QuoteType quoteType;
		LT::Str defaultTypeStr(defaultType.asString()), quoteTypeStr;
		permissive_extract<LT::Str>(instrumentParametersTable.table, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, QUOTETYPE), quoteTypeStr, defaultTypeStr);
		if (quoteTypeStr.empty())
			quoteType = defaultType;
		else
			quoteType = QuoteType(quoteTypeStr);

		SettledBondPtr setBond = createSetBondForInstrument(instrumentParametersTable.table, curveParametersTable, *buildDate, extraInfoTable, BondType::InflationBond);

		// !endDate is not bond maturity date (which could be on holiday), it should be the bond last payment date
		LT::date endDate = setBond->m_xbond->getLastPaymentDate().getAsLTdate();
		LT::date startDate = setBond->m_contractDate;

		std::string timeLag, interp;
		try
		{
			const IDeA::ILRate& ilRate = dynamic_cast<const IDeA::ILRate&>(*setBond->m_xbond->m_bond->m_bondDefinition->m_rate);
			timeLag = ilRate.m_refIndex1->m_timeLag.asString().string();
			if(timeLag[0] != '-')
			{
				timeLag = "-" + timeLag;
			}
			interp	= ilRate.m_refIndex1->m_resetType.asString().data();
		}
		catch (const std::bad_cast& e)
		{
			LTQC_THROW( IDeA::ModelException, e.what() << ": InflationBondInstrument::createInstruments: the instrument is not an inflation bond." );
		}

		tr1::function<const ILIndexArg(const LT::date&, const LT::date&)> func = std::tr1::bind(getInflationIndexArguments, _1, _2, timeLag, interp);			
		FixedLegILAdjustedPtr fixLegAdj(new FixedLegAdjusted<InflationIndexArguments>(createFixedLegArgFromSetBond(setBond),func));

		// create redemption IL index
		const ILIndexArg ilArg = func(startDate, endDate);
		InflationIndexPtr redepmtionILIndex(InflationIndex::create
							(InflationIndexArguments(ilArg.forward1Time, ilArg.forward2Time, ilArg.weight, endDate)));
		return CalibrationInstrumentPtr(new InflationBondInstrument(1.0, endDate.toDDMMMYYYYString(), startDate, endDate, quoteType, setBond, redepmtionILIndex, fixLegAdj));
	}

	const double InflationBondInstrument::getMarketPrice() const
	{
		return m_setBond->getPerBondNominalDirtyPrice(getRate(), m_quoteType);
	}

	const double InflationBondInstrument::computeModelPrice( const BaseModelPtr model) const
	{
		if(!m_dependentModel)
			initialize(*model);

		const double couponLegValue = m_fixedLegAdj->getValue(*model) * m_coupon * m_notional;
		const double redemptionValue = m_notional * m_dependentModel->getDiscountFactor(m_endDateTime) * m_redemptionILIndex->getValue(*model);

		// divided by base index and reverted back the valueDate pv to settlementDate price:
		return (couponLegValue + redemptionValue) / (m_baseIndexValue * m_dependentModel->getDiscountFactor(m_settleToTradeTime));
	}

	double InflationBondInstrument::computePV( const BaseModelPtr model )
	{
		if(!m_dependentModel)
			initialize(*model);

		return getResidual(model) * m_dependentModel->getDiscountFactor(m_settleToTradeTime);
	}

	double InflationBondInstrument::computeBPV( const BaseModelPtr model )
	{
		if(!m_dependentModel)
			initialize(*model);

		return oneBasisPoint() * m_dependentModel->getDiscountFactor(m_settleToTradeTime) * calculateRateDerivative(model);
	}

	void InflationBondInstrument::accumulateGradient( BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd )
	{
		if(!m_dependentModel)
			initialize(baseModel);

		const double mul = m_notional / (m_baseIndexValue * m_dependentModel->getDiscountFactor(m_settleToTradeTime));
		const double mulFxLegAdj= mul * multiplier * m_coupon;
		const double mulIndex	= mul * multiplier * m_dependentModel->getDiscountFactor(m_endDateTime);

		m_fixedLegAdj->accumulateGradient(baseModel, mulFxLegAdj, gradientBegin, gradientEnd);
		m_redemptionILIndex->accumulateGradient(baseModel, mulIndex, gradientBegin, gradientEnd);
	}

	void InflationBondInstrument::accumulateGradient( BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, const CurveTypeConstPtr& curveType )
	{
		if(!m_dependentModel)
			initialize(baseModel);

		const double mul = m_notional / (m_baseIndexValue * m_dependentModel->getDiscountFactor(m_settleToTradeTime));
		const double mulFxLegAdj= mul * multiplier * m_coupon;
		const double mulIndex	= mul * multiplier * m_dependentModel->getDiscountFactor(m_endDateTime);

		m_fixedLegAdj->accumulateGradient(baseModel, mulFxLegAdj, gradientBegin, gradientEnd, curveType);
		m_redemptionILIndex->accumulateGradient(baseModel, mulIndex, gradientBegin, gradientEnd, curveType);
	}
	
	void InflationBondInstrument::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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

	void InflationBondInstrument::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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

	double InflationBondInstrument::calculateRateDerivative( const BaseModelPtr& model ) const
	{
		// TODO: temporary solution:
		const double shift = 1E-10;
		return (  m_setBond->getPerBondNominalDirtyPrice(getRate() + shift, m_quoteType) 
				- m_setBond->getPerBondNominalDirtyPrice(getRate()- shift, m_quoteType)
				) / (-2.0 * shift);
	}

	void InflationBondInstrument::accumulateRateDerivativeGradient( const BaseModel& model, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd ) const
	{
		// do nothing
	}

	void InflationBondInstrument::accumulateRateDerivativeGradient( const BaseModel& model, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, const CurveTypeConstPtr& curveType ) const
	{
		// do nothing
	}

	FlexYCF::ICloneLookupPtr InflationBondInstrument::cloneWithLookup( CloneLookup& lookup ) const
	{
		return CalibrationInstrumentPtr(new InflationBondInstrument(*this, lookup));
	}

	void InflationBondInstrument::finishCalibration( const BaseModelPtr model )
	{
		CalibrationInstrument::finishCalibration(model);
		m_fixedLegAdj->cleanupCashFlows();
	}

	void InflationBondInstrument::reloadInternalState( const CalibrationInstrumentPtr& src )
	{ 
		const InflationBondInstrumentPtr ourTypeSrc=std::tr1::static_pointer_cast<InflationBondInstrument>(src);
		if(!ourTypeSrc)
		{
			LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
		}

		CalibrationInstrument::reloadInternalState(src);
	}

	void InflationBondInstrument::update()
	{
		m_fixedLegAdj->update();
		m_redemptionILIndex->update();
	}

	double InflationBondInstrument::getDifferenceWithNewRate( const LTQuant::GenericData& instrumentListData ) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<InflationBondInstrument>(), IDeA_KEY(BONDINSTRUMENT, ISIN), IDeA_KEY(BONDINSTRUMENT, QUOTE));
	}

	double InflationBondInstrument::computeParRate( const BaseModelPtr& model )
	{
		double nominalDirtyPrice = computeModelPrice(model);
		return m_setBond->getQuoteFromPerBondNominalDirtyPrice(nominalDirtyPrice, m_quoteType);
	}

	void InflationBondInstrument::updateInstruments( CalibrationInstruments& instrumentList, LTQuant::GenericDataPtr instrumentTable, size_t* instrumentIndex )
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

	double InflationBondInstrument::getLastRelevantTime() const
	{
		return m_redemptionILIndex->getArguments().getForward2Time();
	}

	void InflationBondInstrument::initialize( const BaseModel& baseModel ) const
	{
		LT::Str market, asset;
		if( !baseModel.getDependentMarketData() )
		{
			LTQC_THROW( IDeA::ModelException, "InflationBondInstrument: unable to find any dependencies");
		}
		for (size_t i = 1; i < baseModel.getDependentMarketData()->table->rowsGet(); ++i) 
		{
			AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
			LT::Str asset = IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 

			if (adType == IDeA::AssetDomainType::IR )
			{
				asset = IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
				market = IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1); 
				if(baseModel.hasDependentModel(IRAssetDomain(asset, market)))
				{
					m_dependentModel = baseModel.getDependentModel(IRAssetDomain(asset, market));
					break;
				}
			}
		}

		if(!m_dependentModel)
		{
			LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model" );
		}

		m_fixedLegAdj->setDependentModel(m_dependentModel);
	}

	void InflationBondInstrument::fillCashFlowsTable( LTQuant::GenericData& cashFlowsTable ) const
	{
		// create a redemption flow schedule
		std::string redemptionLeg = "Redemption Leg";
		const LTQuant::GenericDataPtr redemptionFlow(new LTQuant::GenericData(redemptionLeg, 0));
		redemptionFlow->set<LT::date>("Settlement Date", 0, m_setBond->getSettlementDate().getAsLTdate());
		redemptionFlow->set<LT::date>("Maturity Date", 0, m_setBond->m_xbond->getMaturityDate().getAsLTdate());

		// fill cash flow from fixLegAdj and redemption flow
		cashFlowsTable.set<LTQuant::GenericDataPtr>("Coupon Leg", 0, m_fixedLegAdj->getCashFlows());
		cashFlowsTable.set<LTQuant::GenericDataPtr>(redemptionLeg, 0, redemptionFlow);
	}

	void InflationBondInstrument::fillCashFlowPVsTable( const BaseModel& model, LTQuant::GenericData& cashFlowPVsTable ) const
	{
		if(!m_dependentModel)
			initialize(model);

		// create a redemption leg pv flow
		std::string redemptionLeg = "Redemption Leg";
		const LTQuant::GenericDataPtr redemptionFlowPvTbl(new LTQuant::GenericData(redemptionLeg, 0));
		redemptionFlowPvTbl->set<LT::date>("Settlement Date", 0, m_setBond->getSettlementDate().getAsLTdate());
		redemptionFlowPvTbl->set<LT::date>("Maturity Date", 0, m_setBond->m_xbond->getMaturityDate().getAsLTdate());
		redemptionFlowPvTbl->set<double>("Discount Factor", 0, m_dependentModel->getDiscountFactor(m_endDateTime));
		redemptionFlowPvTbl->set<LT::date>("Payment Date", 0, m_setBond->m_xbond->getLastPaymentDate().getAsLTdate());
		redemptionFlowPvTbl->set<double>("Base Inflation Index", 0, m_baseIndexValue);
		redemptionFlowPvTbl->set<double>("Inflation Index", 0, m_redemptionILIndex->getValue(model));

		// fill cash flow pv table from fixLegAdj and redemption flow
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Coupon Leg", 0, m_fixedLegAdj->computeCashFlowPVs(model));
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>(redemptionLeg, 0, redemptionFlowPvTbl);
	}

}
