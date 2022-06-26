/*****************************************************************************
    
	TenorBasisSwap

	Implementation of TenorBasisSwap

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//FlexYCF
#include "TenorBasisSwap.h"
#include "BaseModel.h"
#include "FlexYcfUtils.h"
#include "AllComponentsAndCaches.h"
#include "GlobalComponentCache.h"
#include "FlexYCFCloneLookup.h"

//	LTQuantLib
#include "Data/GenericData.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"
#include "TenorUtils.h"

#include "ModuleDate/InternalInterface/CalendarFactory.h"

// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "DataExtraction.h"
#include "TradeConventionCalcH.h"
#include "DictionaryManager.h"
#include "IRRate.h"

// LTQC
#include "DayCount.h"
#include "ModuleDate/InternalInterface/DayCounterFactory.h"

using namespace LTQC;
using namespace std;
using namespace IDeA;

namespace
{
	bool shouldBackStubAdjustmentApply(const LTQC::Tenor& maturityTenor,
									   const LTQC::Tenor& leg1FrequencyTenor,
									   const LTQC::Tenor& leg2FrequencyTenor)
	{
		using namespace FlexYCF;
		return shouldLegHaveStub(maturityTenor, leg1FrequencyTenor) || shouldLegHaveStub(maturityTenor, leg2FrequencyTenor);
	}
}

namespace
{
	using namespace LTQuant;
    using namespace ModuleDate;
	using namespace FlexYCF;

	void inputFormatTransformation(const GenericDataPtr inputTable, GenericData& outputTable)
	{
		
		// just finish if we have empty table or not enough headings
        if(inputTable->numItems() < 2 || inputTable->numTags() < 3)
        {
            return;
        }

       
		const GenericDataPtr tenorTable(IDeA::extract<GenericDataPtr>(*inputTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR)));
		const GenericDataPtr maturityTable(IDeA::extract<GenericDataPtr>(*inputTable, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY)));
		const GenericDataPtr valuesTable(IDeA::extract<GenericDataPtr>(*inputTable, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES)));

		const size_t nbTenors(IDeA::numberOfRecords(*tenorTable));
		const size_t nbMaturities(IDeA::numberOfRecords(*maturityTable));

        // perform checks on subtables to ensure the reading code will work
        if(nbTenors == 0 || nbMaturities == 0)
        {
            return;
        }

		// the number of columns of the table containing the tenor basis spreads
		// must be the same as the number of specified tenors
        if(valuesTable->numTags() != nbTenors)
        {
            LT_THROW_ERROR("Wrong number of columns in values table");
        }

        if(valuesTable->numItems() != nbMaturities)
        {
            LT_THROW_ERROR("Wrong number of rows in values table");
        }

		std::vector<LT::Str> descriptions, tenors;
	    for(size_t cnt(0); cnt < nbMaturities; ++cnt)
        {
			string description = IDeA::extract<std::string>(*maturityTable, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY), cnt);
            if(!description.empty())
            {
				descriptions.push_back(description);
			}
		}
		for(size_t tenorCounter = 0; tenorCounter < tenorTable->numItems() - 1; ++tenorCounter)
		{
			const string tenorStr = IDeA::extract<std::string>(*tenorTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR), tenorCounter);
			if( !tenorStr.empty() )
			{	
				tenors.push_back(tenorStr);
			}
		}

		LT::TablePtr basisSwaps = DictionaryManager::getInstance().createTblFromDictionary(IDeA_TAG(TENORBASISSWAPBYTENOR), tenors.size() + 1);
		

		for(size_t tenorCounter = 0; tenorCounter < tenors.size(); ++tenorCounter)
		{
			// LT::TablePtr basisSwapQuotes = DictionaryManager::getInstance().createTblFromDictionary(IDeA_TAG(BASISSWAP), nbMaturities + 1);
			LT::TablePtr basisSwapQuotes( new LT::Table(nbMaturities + 1,2));
			basisSwapQuotes->at(0, 0) = "TENOR";
			basisSwapQuotes->at(0, 1) = "RATE";
			for(size_t matCounter = 0; matCounter < nbMaturities; ++matCounter)
			{      
				double spread = valuesTable->get<double>(tenorCounter, matCounter);

				basisSwapQuotes->at(matCounter + 1, IDeA_PARAM(BASISSWAP, TENOR)) = descriptions[matCounter];
				basisSwapQuotes->at(matCounter + 1, IDeA_PARAM(BASISSWAP, RATE))  = spread;
				
			}
			basisSwaps->at(tenorCounter + 1, IDeA_PARAM(TENORBASISSWAPBYTENOR, TENOR))     = tenors[tenorCounter];
			basisSwaps->at(tenorCounter + 1, IDeA_PARAM(TENORBASISSWAPBYTENOR, BASISSWAP)) = basisSwapQuotes;
		}
		outputTable.table = basisSwaps;
	}
}

namespace FlexYCF
{

	template<>
	const IDeA::DictionaryKey& getKey<TenorBasisSwap>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, TENORBASISSWAP);
	}

    using namespace LTQuant;
    using namespace ModuleDate;
	
	string floorTenor(const std::string requestedTenor)
	{
		static LTQC::Tenor minTenor(0, 0, 0, 1, 0); // Minimum tenor supported is 1M
		LTQC::Tenor requiredTenor(requestedTenor);
		if (requiredTenor < minTenor)
			return minTenor.asTenorString().string();
		else
			return requestedTenor;
	}

	string createTenorBasisName(const LTQC::Tenor& otherTenor)
	{
		return TenorBasisSwap::getName().append(" ").append(otherTenor.asTenorString().string());
	}

    TenorBasisSwapDetails::TenorBasisSwapDetails(const LTQC::Tenor& spotDays, const LTQC::Currency& currency, const LT::Str& index, 
									  const FloatLeg& leg1, 
									  const FloatLeg& leg2, 
									  const FixedLeg& spreadLeg):
										m_spotDays(spotDays), m_currency( currency), m_index(index), 
										m_leg1(leg1),
										m_leg2(leg2),
										m_spreadleg(spreadLeg), m_spreadLegIsShortLeg(false)
	{
		if (leg1.m_isReferenceLeg == leg2.m_isReferenceLeg)
            LTQC_THROW(IDeA::MarketException, "Only one reference leg allowed in Tenor Basis swap");

		finalize();
	}

	TenorBasisSwapDetails::TenorBasisSwapDetails(const LTQC::Tenor& spotDays, const LTQC::Currency& currency, const LT::Str& index, 
									  const FloatLeg& leg1, 
									  const FloatLeg& leg2):
										m_spotDays(spotDays), m_currency( currency), m_index(index), 
										m_leg1(leg1),
										m_leg2(leg2),
										m_spreadLegIsShortLeg(true)
	{
		if (leg1.m_isReferenceLeg == leg2.m_isReferenceLeg)
			LTQC_THROW(IDeA::MarketException, "Only one reference leg allowed in Tenor Basis swap");

		finalize();
	}

	void TenorBasisSwapDetails::setSpreadLeg(const FixedLeg& spreadLeg)
	{
		m_spreadleg = spreadLeg;
		m_spreadLegIsShortLeg = false;
	}

	void TenorBasisSwapDetails::setSpreadLegFromShortLeg()
	{
		const FloatLeg& shortLeg = getShortLeg();
		m_spreadleg.m_accrualBasis = shortLeg.m_accrualBasis;
		m_spreadleg.m_frequency = shortLeg.m_frequency;
		m_spreadleg.m_rollConvention = shortLeg.m_rollConvention;
		m_spreadleg.m_accrualCalendar = shortLeg.m_accrualCalendar;
	}
	void TenorBasisSwapDetails::identifyShortLeg()
	{
		// identify short leg
		m_leg2.m_isShortLeg = m_leg2.m_depRateMktConvention.m_rateTenor.asYearFraction() <= m_leg1.m_depRateMktConvention.m_rateTenor.asYearFraction();
		m_leg1.m_isShortLeg = !m_leg2.m_isShortLeg;
	}

	void TenorBasisSwapDetails::finalize()
	{	
		identifyShortLeg();
		// set spread leg details from short leg if appropriate
		if( m_spreadLegIsShortLeg ) 
		{
			setSpreadLegFromShortLeg();
		}
    }
	

    TenorBasisSwap::TenorBasisSwap(const string& description,
								   const LT::date fixingDate,
								   const LT::date startDate,
								   const LT::date endDate,
								   const LT::date lastDate,
								   const double spread,
								   const TenorBasisSwapDetails& tradeDetails,
								   const LT::date valueDate,
								   const EndDateCalculationType backStubEndDateCalculationType) :
		CalibrationInstrument(spread, createTenorBasisName(tradeDetails.getOtherLeg().m_frequency)/*getKeyName<TenorBasisSwap>().string()*/, description, fixingDate, startDate, lastDate),
		m_referenceTenor(tenorDescToYears(tradeDetails.getBasisReferenceLeg().m_frequency.asTenorString().string())),
		m_otherTenor(tenorDescToYears(tradeDetails.getOtherLeg().m_frequency.asTenorString().string())),
        m_shortTenor(tenorDescToYears(tradeDetails.getShortLeg().m_frequency.asTenorString().string())),
        m_longTenor(tenorDescToYears(tradeDetails.getLongLeg().m_frequency.asTenorString().string())),
        m_spreadLegTenor(tenorDescToYears(tradeDetails.getSpreadLeg().m_frequency.asTenorString().string())),
		m_details(tradeDetails), m_flooredDetails(tradeDetails)
    {
		// rate is added to the short tenor leg
		// basis swap tenor : short leg
		// non-basis swap tenor: long leg
        if(m_longTenor < m_shortTenor)
        {
			LTQC_THROW(IDeA::MarketException, "The spread leg must be a short tenor " << m_shortTenor << " and the non-spread leg a long tenor " << m_longTenor);
        }

		TenorBasisSwapDetails::FloatLeg& shortFlooredLeg = m_flooredDetails.getShortLeg();
		// ************** HACK ***********************************************************
		// N.B. FIXME: This is a HACK: all tenor basis swaps less than 1M floored at 1M
		const std::string flooredShortTenor = floorTenor(shortFlooredLeg.m_frequency.asTenorString().string());
		shortFlooredLeg.m_frequency = LTQC::Tenor(flooredShortTenor);
		shortFlooredLeg.m_depRateMktConvention.m_rateTenor = LTQC::Tenor(shortFlooredLeg.m_frequency);
		TenorBasisSwapDetails::FixedLeg flooredSpreadLeg(m_flooredDetails.getSpreadLeg());
		flooredSpreadLeg.m_frequency = LTQC::Tenor(floorTenor(flooredSpreadLeg.m_frequency.asTenorString().string()));
		m_flooredDetails.setSpreadLeg(flooredSpreadLeg);
		// ************** HACK ***********************************************************


		m_backStubEndDateCalculationType = (shouldBackStubAdjustmentApply(getDescription(), shortFlooredLeg.m_frequency, m_details.getLongLeg().m_frequency)?
											backStubEndDateCalculationType:
											EndDateCalculationType::NotAdjusted);

        //  1.  Short Tenor Leg
		m_shortFloatingLeg		= TenorBasisSwap::createFloatingLeg(shortFlooredLeg, valueDate);
		m_spreadFixedLeg		= TenorBasisSwap::createFixedLeg(m_flooredDetails.getSpreadLeg(), valueDate);

        //  2.  Long Tenor Leg
		m_longFloatingLeg	= TenorBasisSwap::createFloatingLeg(m_details.getLongLeg(), valueDate);
    }
	
    TenorBasisSwap::TenorBasisSwap(const string& description,
								   const LT::date fixingDate,
								   const LT::date startDate,
								   const LT::date endDate,
								   const LT::date lastDate,
								   const double spread,
								   const TenorBasisSwapDetails& tradeDetails,
                                   const LT::date valueDate,
								   const EndDateCalculationType backStubEndDateCalculationType,
                                   GlobalComponentCache& globalComponentCache):
		CalibrationInstrument(spread, createTenorBasisName(tradeDetails.getOtherLeg().m_frequency)/*getKeyName<TenorBasisSwap>()*/, description, fixingDate, startDate, lastDate),
		m_referenceTenor(tenorDescToYears(tradeDetails.getBasisReferenceLeg().m_frequency.asTenorString().string())),
		m_otherTenor(tenorDescToYears(tradeDetails.getOtherLeg().m_frequency.asTenorString().string())),
        m_shortTenor(tenorDescToYears(tradeDetails.getShortLeg().m_frequency.asTenorString().string())),
        m_longTenor(tenorDescToYears(tradeDetails.getLongLeg().m_frequency.asTenorString().string())),
        m_spreadLegTenor(tenorDescToYears(tradeDetails.getSpreadLeg().m_frequency.asTenorString().string())),
		m_details(tradeDetails), m_flooredDetails(tradeDetails)
    {
        // rate is added to the short tenor leg
		// basis swap tenor : short leg
		// non-basis swap tenor: long leg
        if(m_longTenor < m_shortTenor)
        {
			LTQC_THROW(IDeA::MarketException, "The spread leg must be a short tenor " << m_shortTenor << " and the non-spread leg a long tenor " << m_longTenor);
        }

		TenorBasisSwapDetails::FloatLeg& shortFlooredLeg(m_flooredDetails.getShortLeg());
		// ************** HACK ***********************************************************
		// N.B. FIXME: This is a HACK: all tenor basis swaps less than 1M floored at 1M
		const std::string flooredShortTenor = floorTenor(shortFlooredLeg.m_frequency.asTenorString().string());
		shortFlooredLeg.m_frequency = LTQC::Tenor(flooredShortTenor);
		shortFlooredLeg.m_depRateMktConvention.m_rateTenor = LTQC::Tenor(shortFlooredLeg.m_frequency);
		TenorBasisSwapDetails::FixedLeg flooredSpreadLeg(m_flooredDetails.getSpreadLeg());
		flooredSpreadLeg.m_frequency = LTQC::Tenor(floorTenor(flooredSpreadLeg.m_frequency.asTenorString().string()));
		m_flooredDetails.setSpreadLeg(flooredSpreadLeg);
		// ************** HACK ***********************************************************

		if(LTQC::Tenor::isValid(getDescription()))
		{
			m_backStubEndDateCalculationType = (shouldBackStubAdjustmentApply(getDescription(), shortFlooredLeg.m_frequency, m_details.getLongLeg().m_frequency)? backStubEndDateCalculationType: EndDateCalculationType::NotAdjusted);
		}
		else
		{
			m_backStubEndDateCalculationType = backStubEndDateCalculationType;
		}

        //  1.  Short Tenor Leg
		m_shortFloatingLeg		= TenorBasisSwap::createFloatingLeg(shortFlooredLeg, valueDate, &globalComponentCache);
		m_spreadFixedLeg		= TenorBasisSwap::createFixedLeg(m_flooredDetails.getSpreadLeg(), valueDate, &globalComponentCache);
			
        //  2.  Long Tenor Leg
		m_longFloatingLeg	= TenorBasisSwap::createFloatingLeg(m_details.getLongLeg(), valueDate, &globalComponentCache);
	}

	FloatingLegPtr TenorBasisSwap::createFloatingLeg(const TenorBasisSwapDetails::FloatLeg& floatLeg,
													 const LT::date valueDate,
													 GlobalComponentCache* const globalComponentCache) const
	{
		const DayCounterConstPtr basis(LTQC::DayCount::create(floatLeg.m_depRateMktConvention.m_dcm));

		/* --> at instrument level
		//	By default, do NOT adjust the calculation of the end date for the last period of the floating leg
		EndDateCalculationType backStubEndDateCalculationType(EndDateCalculationType::NotAdjusted);

		const LTQC::Tenor maturityTenor(getDescription());
		const LTQC::Tenor frequencyTenor(floatLeg.m_frequency.asString());

		//	Checks if there is stub period. This works for leg tenors of at least 1M
		if(maturityTenor < frequencyTenor || maturityTenor.asMonths() % frequencyTenor.asMonths() != 0)
		{
			backStubEndDateCalculationType = m_backStubEndDateCalculationType;
		}
		*/

		const FloatingLegArguments fltLegArgs(valueDate,
											  getFixingDate(),
											  getStartDate(),
											  getEndDate(),
											  floatLeg.m_frequency.asTenorString().string(),
											  floatLeg.m_accrualCalendar.string(),
											  floatLeg.m_depRateMktConvention,
											  m_backStubEndDateCalculationType,
											  LTQC::Tenor(),
											  ModuleDate::DayCounterConstPtr(ModuleDate::DayCounterFactory::create(floatLeg.m_accrualBasis.asString().data())),
											  floatLeg.m_stubType
											  );

		return (globalComponentCache? globalComponentCache->get(fltLegArgs): FloatingLeg::create(fltLegArgs));				 
	}
	
	
	FixedLegPtr TenorBasisSwap::createFixedLeg(const TenorBasisSwapDetails::FixedLeg& fixLeg,
											   const LT::date valueDate,
											   GlobalComponentCache* const globalComponentCache) const
	{
		const FixedLegArguments fxdLegArgs(valueDate,
										   getFixingDate(),
										   getStartDate(),
										   getEndDate(),
										   fixLeg.m_frequency.asTenorString().string(),
										   fixLeg.m_accrualBasis.asString().data(),
										   fixLeg.m_accrualCalendar.string(),
										   LT::Str(""),
                                           LT::Str(""),
										   LTQC::RollConvMethod::ModifiedFollowing,
										   fixLeg.m_rollRuleConvention,
										   LT::Str("0B"),
										   LT::Str(""),
                                           LT::Str("Next"),
                                           fixLeg.m_stubType
										   );

		return (globalComponentCache? globalComponentCache->get(fxdLegArgs): FixedLeg::create(fxdLegArgs));
	}


	CalibrationInstrumentPtr TenorBasisSwap::create(const LTQuant::GenericData& instrumentParametersTable, 
													const LT::Ptr<LT::date>& buildDate,
													const LTQuant::GenericData& curveParametersTable,
													const LTQuant::GenericDataPtr&)
	{
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(curveParametersTable));
		const IDeA::TenorBasisSwapMktConvention& mktConv( conventions->m_tenorBasisSwaps );
		const IDeA::TenorBasisSwapMktConvention::ReferenceLegMktConvention& refLegConv( mktConv.getReferenceLegMktConv( ) );
		const IDeA::TenorBasisSwapMktConvention::OtherLegMktConvention& otherLegConv( mktConv.getOtherLegMktConv( ) );

		//	Extract the applicable back stub end date calculation type:
		std::string endDateCalcTypeStr;
		IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, TENORBASISBACKSTUBENDDATE), endDateCalcTypeStr,
											  EndDateCalculationType::toString(TenorBasisSwap::defaultBackStubEndDataCalculationType()).data());
		const EndDateCalculationType applicableBackStubEndDateCalculationType(EndDateCalculationType::fromString(endDateCalcTypeStr));
		
		std::string swapRollRule, emptyStr, stubTypeStr;
		bool rollRuleProvided = IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ROLLRULEMETHOD), swapRollRule, emptyStr);
		bool stubTypeProvided = IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STUBTYPE), stubTypeStr, emptyStr);
		
		std::string endDateCalcTypeStr1, endDateCalcTypeStr2;
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATECALCULATIONTYPE1), endDateCalcTypeStr1,EndDateCalculationType::toString(EndDateCalculationType::NotAdjusted).data());
		const EndDateCalculationType endDateCalculationType1(EndDateCalculationType::fromString(endDateCalcTypeStr1));
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATECALCULATIONTYPE2), endDateCalcTypeStr2,EndDateCalculationType::toString(EndDateCalculationType::NotAdjusted).data());
		const EndDateCalculationType endDateCalculationType2(EndDateCalculationType::fromString(endDateCalcTypeStr2));

		// LEG1
		// Tenor
		std::string tenor1 = IDeA::extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, TENOR1));
        const LTQC::Tenor refLegFrequency( tenor1 );

		// basis
		std::string basis1;
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ACCRUALBASIS1), basis1, refLegConv.m_accrualBasis.asString().data());
        const LTQC::DayCountMethod refLegDCM = !basis1.empty( ) ? LTQC::DayCountMethod( basis1 ) : refLegConv.m_accrualBasis;

        IDeA::DepositRateMktConvention refDepoConvention( mktConv.m_spotDays, mktConv.m_currency, mktConv.m_index, refLegConv.m_underlyingRate->m_dayCount.getDayCountMethodEnum(), refLegFrequency, refLegConv.m_underlyingRate->m_accrualValueCalendar, 
                                                          refLegConv.m_underlyingRate->m_rollConvMethod, rollRuleProvided ?  LTQC::RollRuleMethod(swapRollRule) : refLegConv.m_underlyingRate->m_rollRuleMethod, refLegConv.m_underlyingRate->m_fixingCalendar, LTQC::RollConvMethod(LTQC::RollConvMethod::None));
		refDepoConvention.m_endDateCalculationType = endDateCalculationType1;

        TenorBasisSwapDetails::FloatLeg refLeg( refDepoConvention, refLegFrequency, refLegDCM, refLegConv.m_rollConvention, refLegConv.m_accrualCalendar, refLegConv.m_underlyingRate->m_fixingCalendar, true );
		if(stubTypeProvided)
		{
			refLeg.m_stubType = LTQC::StubType(stubTypeStr);
		}
		// LEG2

		// Tenor
		const std::string tenor2(IDeA::extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, TENOR2)));
        const LTQC::Tenor otherLegFrequency( tenor2 );

		// basis
		std::string basis2;
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ACCRUALBASIS2), basis2, otherLegConv.m_accrualBasis.asString().data());
        const LTQC::DayCountMethod otherLegDCM = !basis2.empty( ) ? LTQC::DayCountMethod( basis2 ) : otherLegConv.m_accrualBasis;
        const bool isReferenceLegShorter = refLegFrequency < otherLegFrequency;

        IDeA::DepositRateMktConvention otherDepoConvention = refDepoConvention;
        otherDepoConvention.m_rateTenor = otherLegFrequency;
		otherDepoConvention.m_endDateCalculationType = endDateCalculationType2;

        TenorBasisSwapDetails::FloatLeg otherLeg( otherDepoConvention, otherLegFrequency, otherLegDCM, otherLegConv.m_rollConvention, otherLegConv.m_accrualCalendar, otherLegConv.m_fixingCalendar, false );
		otherLeg.m_stubType = refLeg.m_stubType;

		const IDeA::TenorBasisSwapMktConvention::SpreadLegMktConvention& spreadLegConv = mktConv.getSpreadLegMktConv();
		

		// SPREAD LEG
        std::string defaultSpreadTenor( ( spreadLegConv.m_spreadIsShortLeg ? LT::Str( ) : spreadLegConv.m_frequency.asTenorString() ).string( ) );
		std::string spreadTenor;
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPREADTENOR), spreadTenor, defaultSpreadTenor );
        const LTQC::Tenor spreadLegFrequency = !spreadTenor.empty( ) ? LTQC::Tenor( spreadTenor ) : ( isReferenceLegShorter ? refLegFrequency : otherLegFrequency );

		// basis
        const std::string defaultSpreadBasis( ( spreadLegConv.m_spreadIsShortLeg ? LT::Str( ) : spreadLegConv.m_accrualBasis.asString( ) ).string( ) );
		std::string spreadBasis;
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPREADACCRUALBASIS), spreadBasis, defaultSpreadBasis);
        const LTQC::DayCountMethod spreadLegDCM = !spreadBasis.empty( ) ? LTQC::DayCountMethod(spreadBasis) : ( isReferenceLegShorter ? refLegDCM : otherLegDCM );

        TenorBasisSwapDetails::FixedLeg spreadLeg( spreadLegFrequency, spreadLegDCM, refLeg.m_rollConvention, refLeg.m_accrualCalendar );
		if(rollRuleProvided)
		{
			spreadLeg.m_rollRuleConvention = LTQC::RollRuleMethod(swapRollRule);
		}
		if(stubTypeProvided)
		{
			spreadLeg.m_stubType = LTQC::StubType(stubTypeStr);
		}
		std::string spotDaysStr;
		bool spotDaysProvided = IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPOTDAYS), spotDaysStr, mktConv.m_spotDays.asTenorString().string());
        TenorBasisSwapDetails swapDetails( spotDaysProvided ? LTQC::Tenor(spotDaysStr) : mktConv.m_spotDays, mktConv.m_currency, mktConv.m_index, refLeg, otherLeg, spreadLeg );

		std::string maturity;
		LT::date fixingDate, startDate, endDate, actualBuildDate;
		setMaturityAndDates(instrumentParametersTable, buildDate, swapDetails.getShortLeg().m_fixingCalendar, swapDetails.getShortLeg().m_depRateMktConvention.m_accrualValueCalendar, swapDetails.m_spotDays, maturity, actualBuildDate, fixingDate, startDate, endDate);

		return CalibrationInstrumentPtr(new TenorBasisSwap(maturity, 
														   fixingDate,
														   startDate,
														   endDate,
														   endDate,
														   0.0,
														   swapDetails,
														   actualBuildDate,
														   applicableBackStubEndDateCalculationType));
	}

    void TenorBasisSwap::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_shortFloatingLeg->cleanupCashFlows();
        m_longFloatingLeg->cleanupCashFlows();
        m_spreadFixedLeg->cleanupCashFlows();
    }

    void TenorBasisSwap::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
        const TenorBasisSwapPtr ourTypeSrc=std::tr1::static_pointer_cast<TenorBasisSwap>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        CalibrationInstrument::reloadInternalState(src);
    }

	bool TenorBasisSwap::hasNewMarketDataFormat(const LTQuant::GenericDataPtr& instrumentTable)
	{
		return hasNewMarketDataFormat(instrumentTable->table);
	}
	bool TenorBasisSwap::hasNewMarketDataFormat(const LT::TablePtr& instrumentTable)
	{
		return instrumentTable->colsGet() == 2;
	}

	void TenorBasisSwap::createInstruments(CalibrationInstruments& instruments,
                                           LTQuant::GenericDataPtr instrumentTable,
                                           LTQuant::GenericDataPtr masterTable,
                                           GlobalComponentCache& globalComponentCache,
                                           const LTQuant::PriceSupplierPtr)
    {
		if(hasNewMarketDataFormat(instrumentTable))
		{
			createInstrumentsNewFormat(instruments, instrumentTable, masterTable, globalComponentCache, LTQuant::PriceSupplierPtr());
		}
		else
		{
			createInstrumentsOldFormat(instruments, instrumentTable, masterTable, globalComponentCache, LTQuant::PriceSupplierPtr());
		}
	}


    void TenorBasisSwap::createInstrumentsOldFormat(CalibrationInstruments& instruments,
                                           LTQuant::GenericDataPtr instrumentTable,
                                           LTQuant::GenericDataPtr masterTable,
                                           GlobalComponentCache& globalComponentCache,
                                           const LTQuant::PriceSupplierPtr)
    {
		// just finish if we have empty table or not enough headings
        if(instrumentTable->numItems() < 2 || instrumentTable->numTags() < 3)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));

		const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));				
		const IDeA::TenorBasisSwapMktConvention& mktConv = conventions->m_tenorBasisSwaps;
		const IDeA::TenorBasisSwapMktConvention::ReferenceLegMktConvention& refLegConv = mktConv.getReferenceLegMktConv();
        const IDeA::DepositRateMktConvention refDepoConvention( mktConv.m_spotDays, mktConv.m_currency, mktConv.m_index, refLegConv.m_underlyingRate->m_dayCount.getDayCountMethodEnum(), refLegConv.m_underlyingRate->m_tenor, refLegConv.m_underlyingRate->m_accrualValueCalendar, 
                                                          refLegConv.m_underlyingRate->m_rollConvMethod, refLegConv.m_underlyingRate->m_rollRuleMethod, refLegConv.m_underlyingRate->m_fixingCalendar, LTQC::RollConvMethod(LTQC::RollConvMethod::None) );

        TenorBasisSwapDetails::FloatLeg refLeg( refDepoConvention, refLegConv.m_underlyingRate->m_tenor, refLegConv.m_accrualBasis,
					                           refLegConv.m_rollConvention, refLegConv.m_accrualCalendar, refLegConv.m_underlyingRate->m_fixingCalendar, true );
        const IDeA::TenorBasisSwapMktConvention::OtherLegMktConvention& otherLegConv = mktConv.getOtherLegMktConv();

		//	Extract the applicable back stub end date calculation type:
		std::string endDateCalcTypeStr;
		IDeA::permissive_extract<std::string>(parametersTable, IDeA_KEY(YC_CURVEPARAMETERS, TENORBASISBACKSTUBENDDATE), endDateCalcTypeStr,
											  EndDateCalculationType::toString(TenorBasisSwap::defaultBackStubEndDataCalculationType()).data());
		const EndDateCalculationType applicableBackStubEndDateCalculationType(EndDateCalculationType::fromString(endDateCalcTypeStr));
		
        const DictionaryKey& tenorBasisSwapDetailsTenor = IDeA_KEY(TENORBASISSWAPDETAILS, TENOR);
        const DictionaryKey& tenorBasisSwapDetailsMaturity = IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY);

		const GenericDataPtr tenorTable(IDeA::extract<GenericDataPtr>(*instrumentTable, tenorBasisSwapDetailsTenor));
		const GenericDataPtr maturityTable(IDeA::extract<GenericDataPtr>(*instrumentTable, tenorBasisSwapDetailsMaturity));
		const GenericDataPtr valuesTable(IDeA::extract<GenericDataPtr>(*instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES)));

		const size_t nbTenors(IDeA::numberOfRecords(*tenorTable));
		const size_t nbMaturities(IDeA::numberOfRecords(*maturityTable));

        // perform checks on subtables to ensure the reading code will work
        if(nbTenors == 0 || nbMaturities == 0)
        {
            LT_THROW_ERROR("Invalid Tenor basis structure, empty tenors or maturities");
        }

		// the number of columns of the table containing the tenor basis spreads
		// must be the same as the number of specified tenors
        if(valuesTable->numTags() != nbTenors)
        {
            LT_THROW_ERROR("Wrong number of columns in values table");
        }

        if(valuesTable->numItems() != nbMaturities)
        {
            LT_THROW_ERROR("Wrong number of rows in values table");
        }

		LT::date startDate = IDeA::TradeConventionCalcH::getSpotDate(Date(valueDate), refLegConv.m_underlyingRate->m_fixingCalendar, refLegConv.m_accrualCalendar, mktConv.m_spotDays).getAsLTdate();
		LT::date fixingDate(valueDate);
        LT::date endDate;

		// Calc maturity dates
		std::vector<LT::date> matDates;
		std::vector<std::string> descriptions;
		CalendarPtr accCalendar = CalendarFactory::create(refLegConv.m_accrualCalendar.string());
        for(size_t cnt(0); cnt < nbMaturities; ++cnt)
        {
			const string description = IDeA::extract<std::string>(*maturityTable, tenorBasisSwapDetailsMaturity, cnt);
            
            if(!description.empty())    // to skip blank lines
            {
                endDate = addDatePeriod(startDate, description, accCalendar);
				matDates.push_back(endDate);
				descriptions.push_back(description);
			}
		}

        const IDeA::TenorBasisSwapMktConvention::SpreadLegMktConvention& spreadLegConv = mktConv.getSpreadLegMktConv( );
        // build instruments
		for(size_t cnt(0), nDates=matDates.size(); cnt < nDates; ++cnt)
		{
			for(size_t cnt2(0); cnt2 < tenorTable->numItems() - 1; ++cnt2)
			{
				const string tenorStr = IDeA::extract<std::string>(*tenorTable, tenorBasisSwapDetailsTenor, cnt2);

				if( !tenorStr.empty( ) ) // skip empty rows
				{
					// build trade details
                    LTQC::Tenor tenor( tenorStr );
                    IDeA::DepositRateMktConvention otherDepoConvention = refDepoConvention;
                    otherDepoConvention.m_rateTenor = tenor;
                    TenorBasisSwapDetails::FloatLeg otherLeg( otherDepoConvention, tenor, refLegConv.m_accrualBasis, otherLegConv.m_rollConvention, otherLegConv.m_accrualCalendar, otherLegConv.m_fixingCalendar, false );
                    TenorBasisSwapDetails thisSwapDetails = TenorBasisSwapDetails( mktConv.m_spotDays, mktConv.m_currency, mktConv.m_index, refLeg, otherLeg );
                        
                    // If the default spread leg in the market convention is a custom one, the tenor basis swap details need to account for that
                    if( !spreadLegConv.m_spreadIsShortLeg )
                    {
                        const TenorBasisSwapDetails::FixedLeg spreadLeg( spreadLegConv.m_frequency, spreadLegConv.m_accrualBasis, spreadLegConv.m_rollConvention, spreadLegConv.m_accrualCalendar );
                        thisSwapDetails.setSpreadLeg( spreadLeg );
                    }
				
					// build instruments for each maturity
					double spread(valuesTable->get<double>(cnt2, cnt));
					CalibrationInstrumentPtr instrument( new TenorBasisSwap( descriptions[cnt], 
																				fixingDate,
																				startDate,
																				matDates[cnt],
																				matDates[cnt],
																				spread,
																				thisSwapDetails,
																				valueDate,
																				applicableBackStubEndDateCalculationType,
																				globalComponentCache) 
														);
					instruments.add(instrument);
				}
			}
		}

    }

	void TenorBasisSwap::createInstrumentsNewFormat(CalibrationInstruments& instruments,
                                           LTQuant::GenericDataPtr instrumentsTable,
                                           LTQuant::GenericDataPtr masterTable,
                                           GlobalComponentCache& globalComponentCache,
                                           const LTQuant::PriceSupplierPtr)
    {
		// just finish if we have empty table or not enough headings
		LT::TablePtr table = instrumentsTable->table;
		size_t rows = table->rowsGet();
        if(rows < 2)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));

		const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));				
		const IDeA::TenorBasisSwapMktConvention& mktConv = conventions->m_tenorBasisSwaps;
		const IDeA::TenorBasisSwapMktConvention::ReferenceLegMktConvention& refLegConv = mktConv.getReferenceLegMktConv();
        IDeA::DepositRateMktConvention refDepoConvention( mktConv.m_spotDays, mktConv.m_currency, mktConv.m_index, refLegConv.m_underlyingRate->m_dayCount.getDayCountMethodEnum(), refLegConv.m_underlyingRate->m_tenor, refLegConv.m_underlyingRate->m_accrualValueCalendar, 
                                                          refLegConv.m_underlyingRate->m_rollConvMethod, refLegConv.m_underlyingRate->m_rollRuleMethod, refLegConv.m_underlyingRate->m_fixingCalendar, LTQC::RollConvMethod(LTQC::RollConvMethod::None) );

       
		//	Extract the applicable back stub end date calculation type:
		std::string endDateCalcTypeStr;
		IDeA::permissive_extract<std::string>(parametersTable, IDeA_KEY(YC_CURVEPARAMETERS, TENORBASISBACKSTUBENDDATE), endDateCalcTypeStr,EndDateCalculationType::toString(TenorBasisSwap::defaultBackStubEndDataCalculationType()).data());
		const EndDateCalculationType applicableBackStubEndDateCalculationType(EndDateCalculationType::fromString(endDateCalcTypeStr));
		
	
        const IDeA::TenorBasisSwapMktConvention::SpreadLegMktConvention& spreadLegConv = mktConv.getSpreadLegMktConv( );
		
		size_t k1 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
		size_t k2 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());

		for(size_t row = 1; row < rows; ++row)
		{
			LT::TablePtr basisSwapTable = table->at(row,k2);
			LT::Str tenorStr = table->at(row,k1);
			std::string tenorDescription(tenorStr.cStr());
			LTQuant::GenericDataPtr swapTable(new GenericData(basisSwapTable));

			LTQC::Tenor otherTenor(tenorStr);

			for(size_t i = 0; i < IDeA::numberOfRecords(*swapTable); ++i)
			{

				LT::date endDate;
				std::string description, emptyStr;
				bool foundTenor = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, TENOR), i, description, emptyStr);	
				bool foundEndDate = IDeA::permissive_extract<LT::date>(*swapTable, IDeA_KEY(BASISSWAP, ENDDATE), i, endDate, LT::date() );
				if(foundTenor && foundEndDate)
					LTQC_THROW(IDeA::MarketException, "Tenor or end Date have to be provided but not both");
			
				if(foundTenor || foundEndDate)
				{
					std::string spotDaysStr;
					bool spotDaysProvided = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, SPOTDAYS), i, spotDaysStr, mktConv.m_spotDays.asTenorString().string());
					LT::date startDate = IDeA::TradeConventionCalcH::getSpotDate(Date(valueDate), refLegConv.m_underlyingRate->m_fixingCalendar, refLegConv.m_accrualCalendar, spotDaysProvided ? LTQC::Tenor(spotDaysStr) : mktConv.m_spotDays).getAsLTdate();
					LT::date fixingDate(valueDate);

					const double swapRate(IDeA::extract<double>(*swapTable, IDeA_KEY(BASISSWAP, RATE), i));
					std::string spreadTenor, floatingTenor, spreadBasisName, indexName, accrualCalendarStr, floatingBasis1, floatingBasis2, stubTypeStr, swapRollRule, rollRuleStr;

					bool accrualCalendarProvided = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, ACCRUALCALENDAR),  i, accrualCalendarStr, refLegConv.m_accrualCalendar.string());
					bool floatingBasis1Provided = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, ACCRUALBASIS1),       i, floatingBasis1, refLegConv.m_accrualBasis.asString().data());
					bool floatingBasis2Provided = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, ACCRUALBASIS2),       i, floatingBasis2, refLegConv.m_accrualBasis.asString().data());
					
					bool stubTypeProvided = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, STUBTYPE),         i,   stubTypeStr, string());
					bool rollRuleProvided = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, ROLLRULEMETHOD), i, swapRollRule, rollRuleStr);
					
					
					if(rollRuleProvided) refDepoConvention.m_rollRuleConvention = LTQC::RollRuleMethod(swapRollRule);
					
					std::string endDateCalcTypeStr1, endDateCalcTypeStr2;
					IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, ENDDATECALCULATIONTYPE1), i, endDateCalcTypeStr1,EndDateCalculationType::toString(EndDateCalculationType::NotAdjusted).data());
					const EndDateCalculationType endDateCalculationType1(EndDateCalculationType::fromString(endDateCalcTypeStr1));
					IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, ENDDATECALCULATIONTYPE2), i, endDateCalcTypeStr2,EndDateCalculationType::toString(EndDateCalculationType::NotAdjusted).data());
					const EndDateCalculationType endDateCalculationType2(EndDateCalculationType::fromString(endDateCalcTypeStr2));
					refDepoConvention.m_endDateCalculationType = endDateCalculationType1;

					TenorBasisSwapDetails::FloatLeg refLeg( refDepoConvention, refLegConv.m_underlyingRate->m_tenor, refLegConv.m_accrualBasis, refLegConv.m_rollConvention, refLegConv.m_accrualCalendar, refLegConv.m_underlyingRate->m_fixingCalendar, true );
					if(stubTypeProvided) refLeg.m_stubType = LTQC::StubType(stubTypeStr);
					if(accrualCalendarProvided) refLeg.m_accrualCalendar =  accrualCalendarStr;
					if(floatingBasis1Provided) refLeg.m_accrualBasis = LTQC::DayCountMethod(floatingBasis1);

					const IDeA::TenorBasisSwapMktConvention::OtherLegMktConvention& otherLegConv = mktConv.getOtherLegMktConv();
                
					CalendarPtr accCalendar = CalendarFactory::create(accrualCalendarStr);
					bool foundStartDate = IDeA::permissive_extract<LT::date>(*swapTable, IDeA_KEY(BASISSWAP, STARTDATE), i, startDate, startDate );
					if ( !foundEndDate )
					{
						endDate = addDatePeriod(startDate, description, accCalendar);
					}
					else
					{
						description = endDate.toDDMMMYYYYString();
					}
					

                    IDeA::DepositRateMktConvention otherDepoConvention = refDepoConvention;
                    otherDepoConvention.m_rateTenor = otherTenor;
					otherDepoConvention.m_endDateCalculationType = endDateCalculationType2;
                    
					TenorBasisSwapDetails::FloatLeg otherLeg( otherDepoConvention, otherTenor, refLegConv.m_accrualBasis, otherLegConv.m_rollConvention, otherLegConv.m_accrualCalendar, otherLegConv.m_fixingCalendar, false );
					otherLeg.m_stubType = refLeg.m_stubType;
					if(accrualCalendarProvided) otherLeg.m_accrualCalendar = refLeg.m_accrualCalendar;
					if(floatingBasis2Provided) otherLeg.m_accrualBasis = LTQC::DayCountMethod(floatingBasis2);

                    TenorBasisSwapDetails thisSwapDetails = TenorBasisSwapDetails( LTQC::Tenor(spotDaysStr), mktConv.m_currency, mktConv.m_index, refLeg, otherLeg );
                 
					bool spreadTenorProvided = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, SPREADTENOR), i, spreadTenor, thisSwapDetails.getSpreadLeg().m_frequency.asString().data());
					bool spreadBasisProvided = IDeA::permissive_extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, SPREADBASIS), i, spreadBasisName, thisSwapDetails.getSpreadLeg().m_accrualBasis.asString().data());
                    
					TenorBasisSwapDetails::FixedLeg spreadLeg = thisSwapDetails.getSpreadLeg();
					
					if(rollRuleProvided) spreadLeg.m_rollRuleConvention = LTQC::RollRuleMethod(swapRollRule);
					if(stubTypeProvided) spreadLeg.m_stubType = LTQC::StubType(stubTypeStr);
					if(spreadTenorProvided)  spreadLeg.m_frequency = LTQC::Tenor(spreadTenor);
					if(spreadBasisProvided) spreadLeg.m_accrualBasis = LTQC::DayCountMethod(spreadBasisName);
					spreadLeg.m_accrualCalendar = refLeg.m_accrualCalendar;
					spreadLeg.m_rollConvention = refLeg.m_rollConvention;
                    thisSwapDetails.setSpreadLeg( spreadLeg );
                   	
					LT::date lastDate = endDate;
					IDeA::permissive_extract<LT::date>(*swapTable, IDeA_KEY(BASISSWAP, LASTDATE), i, lastDate, lastDate);
					
					CalibrationInstrumentPtr instrument( new TenorBasisSwap(description, fixingDate, startDate, endDate, lastDate, swapRate, thisSwapDetails, valueDate, applicableBackStubEndDateCalculationType, globalComponentCache) );
					instruments.add(instrument);
				}
			}
		}

	}
    void TenorBasisSwap::updateInstruments(CalibrationInstruments& instrumentList, 
                                              LTQuant::GenericDataPtr instrumentTable, 
                                              size_t* instrumentIndex)
    {
		if(!hasNewMarketDataFormat(instrumentTable))
		{
			// just finish if we have empty table or not enough headings
			if(instrumentTable->numItems() < 2 || instrumentTable->numTags() < 3)
			{
				return;
			}

			const GenericDataPtr tenorTable(IDeA::extract<GenericDataPtr>(*instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR)));
			const GenericDataPtr maturityTable(IDeA::extract<GenericDataPtr>(*instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY)));
			const GenericDataPtr valuesTable(IDeA::extract<GenericDataPtr>(*instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES)));

			const size_t nbTenors(IDeA::numberOfRecords(*tenorTable));
			const size_t nbMaturities(IDeA::numberOfRecords(*maturityTable));


			// perform checks on subtables to ensure the reading code will work
			if(nbTenors == 0 || nbMaturities == 0)
			{
				LT_THROW_ERROR("Invalid Tenor basis structure, empty tenors or maturities");
			}

			if(valuesTable->numTags() != nbTenors)
			{
				LT_THROW_ERROR("Wrong number of columns in values table");
			}

			if(valuesTable->numItems() != nbMaturities)
			{
				LT_THROW_ERROR("Wrong number of rows in values table");
			}

			TenorBasisSwapPtr tbs;

			for(size_t cnt(0); cnt < nbMaturities; ++cnt)
			{
				const string description = IDeA::extract<std::string>(*maturityTable, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY), cnt);
				if(!description.empty())
				{
					for(size_t cnt2(0); cnt2 < nbTenors; ++cnt2)
					{
						const string tenor = IDeA::extract<std::string>(*tenorTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR), cnt2);

						tbs = std::tr1::dynamic_pointer_cast<TenorBasisSwap>(instrumentList[*instrumentIndex]);
					
						if(!tenor.empty() && tbs != 0)
						{
							double spread(valuesTable->get<double>(cnt2, cnt));
							instrumentList[*instrumentIndex]->setRate(spread);
							++(*instrumentIndex);
						}
					}
				}
			}
		}
		else
		{
			TenorBasisSwapPtr tbs;
			LT::TablePtr table = instrumentTable->table;
			size_t rows = table->rowsGet();
			size_t k1 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
			size_t k2 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());
			
			for(size_t row = 1; row < rows; ++row)
			{
				LT::Str tenorStr = table->at(row,k1);
				if(!tenorStr.empty())
				{
					LT::TablePtr basisSwapTable = table->at(row,k2);
					LTQuant::GenericDataPtr swapTable(new GenericData(basisSwapTable));
					for(size_t i = 0; i < IDeA::numberOfRecords(*swapTable); ++i)
					{
						std::string description = IDeA::extract<std::string>(*swapTable, IDeA_KEY(BASISSWAP, TENOR), i);
						tbs = std::tr1::dynamic_pointer_cast<TenorBasisSwap>(instrumentList[*instrumentIndex]);
						if( !description.empty() && tbs != 0)
						{
							double spread = IDeA::extract<double>(*swapTable, IDeA_KEY(BASISSWAP, RATE), i);
							instrumentList[*instrumentIndex]->setRate(spread);
							++(*instrumentIndex);
						}
					}
				}
			}
		}
    }

	bool TenorBasisSwap::hasSyntheticOtherLeg() const
	{
		const DayCounterConstPtr basis(LTQC::DayCount::create(m_details.getOtherLeg().m_depRateMktConvention.m_dcm));

		//	bucket to the closest tenor before comparing
		return CurveType::DereferenceLess()(CurveType::getFromYearFraction(basis->getDaysOverBasis(getStartDate(), getEndDate())), 
											CurveType::getFromYearFraction(m_otherTenor));
	}

    const double TenorBasisSwap::computeModelPrice(const BaseModelPtr model) const
    {  
        return m_longFloatingLeg->getValue(*model) 
            - m_shortFloatingLeg->getValue(*model) 
            - getRate() * m_spreadFixedLeg->getValue(*model); 
    }
        
    void TenorBasisSwap::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {
        // compute the long tenor leg gradient:
        m_longFloatingLeg->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);

        // subtract the short tenor leg gradient from it:
        m_shortFloatingLeg->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd);

        // compute the short tenor fixed leg gradient, multiply by the basis spread and the subtract to the gradient computed so far:
        m_spreadFixedLeg->accumulateGradient(baseModel, -multiplier * getRate(), gradientBegin, gradientEnd);
    }

	void TenorBasisSwap::accumulateGradient(BaseModel const& baseModel,
											double multiplier,
											GradientIterator gradientBegin,
											GradientIterator gradientEnd,
											const CurveTypeConstPtr& curveType)
	{
		// compute the long tenor leg gradient:
        m_longFloatingLeg->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);

        // subtract the short tenor leg gradient from it:
        m_shortFloatingLeg->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd, curveType);

        // compute the short tenor fixed leg gradient, multiply by the basis spread and the subtract to the gradient computed so far:
        m_spreadFixedLeg->accumulateGradient(baseModel, -multiplier * getRate(), gradientBegin, gradientEnd, curveType);
	}

	void TenorBasisSwap::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
	
	void TenorBasisSwap::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void TenorBasisSwap::update()
    {
        m_shortFloatingLeg->update();
        m_longFloatingLeg->update();
        m_spreadFixedLeg->update();
    }

	double TenorBasisSwap::getVariableInitialGuess(const double flowTime,
												   const BaseModel* const model) const
	{
		LT_LOG << "init guess for T" << getDescription().string()<< ", " 
			<< CurveType::getFromYearFraction(m_shortTenor == m_referenceTenor? m_longTenor : m_shortTenor)->getDescription() << std::endl;
		
		// Note: Assumes the tenor basis swaps create points on the curve whose tenor matches the one of their non-basis swap tenor leg. ie. on the "other" leg
		return model->getVariableValueFromSpineDiscountFactor(flowTime, pow(1.0 + getRate(), flowTime), CurveType::getFromYearFraction(m_otherTenor));
	}

	double TenorBasisSwap::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return -m_spreadFixedLeg->getValue(*model);
	}

	void TenorBasisSwap::accumulateRateDerivativeGradient(const BaseModel& model,
														  double multiplier,
														  GradientIterator gradientBegin,
														  GradientIterator gradientEnd) const
	{
		m_spreadFixedLeg->accumulateGradient(model, -multiplier, gradientBegin, gradientEnd);
	}

	void TenorBasisSwap::accumulateRateDerivativeGradient(const BaseModel& model,
														  double multiplier,
														  GradientIterator gradientBegin,
														  GradientIterator gradientEnd,
														  const CurveTypeConstPtr& curveType) const
	{
		m_spreadFixedLeg->accumulateGradient(model, -multiplier, gradientBegin, gradientEnd, curveType);
	}

	double TenorBasisSwap::computeParRate(const BaseModelPtr& model)
	{
		return (m_longFloatingLeg->getValue(*model) - m_shortFloatingLeg->getValue(*model)) / m_spreadFixedLeg->getValue(*model); 
	}
	
	void TenorBasisSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                      const BaseModel& model,
									  const double multiplier, 
									  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		m_longFloatingLeg->fillRepFlows(assetDomain, model, multiplier, fundingRepFlows);
		m_shortFloatingLeg->fillRepFlows(assetDomain, model, -multiplier, fundingRepFlows);
		m_spreadFixedLeg->fillRepFlows(assetDomain, model, -multiplier * getRate(), fundingRepFlows);
	}

	void TenorBasisSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                      const BaseModel& model,
									  const double multiplier, 
									  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		m_longFloatingLeg->fillRepFlows(assetDomain, model, multiplier, indexRepFlows);
		m_shortFloatingLeg->fillRepFlows(assetDomain, model, -multiplier, indexRepFlows);
		//	No index rep flows for a fixed leg
	}

	double TenorBasisSwap::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{    
        const IDeA::DictionaryKey& instrumentKey = getKey<TenorBasisSwap>();
        LTQuant::GenericDataPtr instrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(instrumentListData, instrumentKey, instrumentData);
		if(!instrumentData)
		{
			LTQC_THROW( LTQC::DataQCException, "Cannot find the instrument table \"" << instrumentKey.getName().string() << "\"" );
		}
		if(!hasNewMarketDataFormat(instrumentData))
		{
            const DictionaryKey& tenorBasisSwapDetailsMaturity = IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY);
            const DictionaryKey& tenorBasisSwapDetailsTenor = IDeA_KEY(TENORBASISSWAPDETAILS, TENOR);

			const LTQuant::GenericDataPtr tenorData(IDeA::extract<GenericDataPtr>(*instrumentData, tenorBasisSwapDetailsTenor));
			const LTQuant::GenericDataPtr maturityData(IDeA::extract<GenericDataPtr>(*instrumentData, tenorBasisSwapDetailsMaturity));
			const LTQuant::GenericDataPtr valuesData(IDeA::extract<GenericDataPtr>(*instrumentData, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES)));

			const size_t nbTenors(IDeA::numberOfRecords(*tenorData));
			const size_t nbMaturities(IDeA::numberOfRecords(*maturityData));

			// perform checks on subtables to ensure the reading code will work
			if(nbTenors == 0 || nbMaturities == 0)
			{
				LT_THROW_ERROR("Invalid Tenor basis structure, empty tenors or maturities");
			}
			if(valuesData->numTags() != nbTenors)
			{
				LT_THROW_ERROR("Wrong number of columns in values table");
			}

			double newRate;
			const LT::Str myTenor(getOtherTenor());
			const LT::Str& myMaturity = getDescription();

			for(size_t i = 0; i < nbMaturities; ++i)
			{
				const LT::Str description(IDeA::extract<std::string>(*maturityData, tenorBasisSwapDetailsMaturity, i));
				for(size_t j = 0, n=tenorData->numItems() - 1; j < n; ++j)
				{
					const LT::Str tenorStr(IDeA::extract<std::string>(*tenorData, tenorBasisSwapDetailsTenor, j));
					if( myTenor.compareCaseless(tenorStr) == 0 && myMaturity.compareCaseless(description) == 0 )
					{
						newRate = valuesData->get<double>(j, i);
						return newRate - getRate();
					}
				}
			}
		}
		else
		{
			LT::TablePtr table = instrumentData->table;
			size_t rows = table->rowsGet();
			size_t k1 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
			size_t k2 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());
			
			LT::Str otherTenor(getOtherTenor());
			LT::Str maturity = getDescription();
			
            const DictionaryKey& basisSwapTenor = IDeA_KEY(BASISSWAP, TENOR);

			for(size_t row = 1; row < rows; ++row)
			{
				LT::Str tenorStr = table->at(row,k1);
				if(otherTenor.compareCaseless(tenorStr) == 0)
				{
					LT::TablePtr basisSwapTable = table->at(row,k2);
					LTQuant::GenericDataPtr swapTable(new GenericData(basisSwapTable));
					for(size_t i = 0; i < IDeA::numberOfRecords(*swapTable); ++i)
					{
						std::string description = IDeA::extract<std::string>(*swapTable, basisSwapTenor, i);
						if( maturity.compareCaseless(description) == 0 )
						{
							double newRate = IDeA::extract<double>(*swapTable, IDeA_KEY(BASISSWAP, RATE), i);
							return newRate - getRate();
						}
					}
				}
			}
		}
		LTQC_THROW( LTQC::DataQCException, "Cannot find a tenor basis swap with maturity \"" << getDescription().string() << "\" and tenor \"" << getOtherTenor() << "\"" );
	}

    ostream& TenorBasisSwap::print(ostream& out) const
    {
        out << "T" << getDescription().string() << ":";
		out << CurveType::getFromYearFraction(m_otherTenor)->getDescription();
        return out;
    }

    /**
        @brief Clone this tenor basis swap.

        Uses a lookup to maintain directed graph relationships.

        @param lookup A lookup of previuosly created clones.
        @return       The clone of this instance.
    */
    ICloneLookupPtr TenorBasisSwap::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new TenorBasisSwap(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    TenorBasisSwap::TenorBasisSwap(TenorBasisSwap const& original, CloneLookup& lookup) : 
        CalibrationInstrument(original),
        m_referenceTenor(original.m_referenceTenor),
        m_otherTenor(original.m_otherTenor),
        m_details(original.m_details),
        m_flooredDetails(original.m_flooredDetails),
        m_shortTenor(original.m_shortTenor),
        m_longTenor(original.m_longTenor),
        m_shortFloatingLeg(lookup.get(original.m_shortFloatingLeg)),
        m_longFloatingLeg(lookup.get(original.m_longFloatingLeg)),
        m_spreadFixedLeg(lookup.get(original.m_spreadFixedLeg)),
        m_backStubEndDateCalculationType(original.m_backStubEndDateCalculationType),
		m_spreadLegTenor(original.m_spreadLegTenor)
    {
    }

    void TenorBasisSwap::fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const
	{
		const std::string shortLegTag("Short Leg");
		const std::string longLegTag("Long Leg");
		const std::string spreadLegTag("Spread Leg");

		cashFlowsTable.set<LTQuant::GenericDataPtr>(shortLegTag, 0, m_shortFloatingLeg->getCashFlows());
		cashFlowsTable.set<LTQuant::GenericDataPtr>(longLegTag, 0, m_longFloatingLeg->getCashFlows());
		cashFlowsTable.set<LTQuant::GenericDataPtr>(spreadLegTag, 0, m_spreadFixedLeg->getCashFlows());
	}

	void TenorBasisSwap::fillCashFlowPVsTable(const BaseModel& model,
											  LTQuant::GenericData& cashFlowPVsTable) const
	{
		const std::string shortLegTag("Short Leg");
		const std::string longLegTag("Long Leg");
		const std::string spreadLegTag("Spread Leg");

		cashFlowPVsTable.set<LTQuant::GenericDataPtr>(shortLegTag, 0, m_shortFloatingLeg->computeCashFlowPVs(model));
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>(longLegTag, 0, m_longFloatingLeg->computeCashFlowPVs(model));
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>(spreadLegTag, 0, m_spreadFixedLeg->computeCashFlowPVs(model));
	}
}   // FlexYCF
