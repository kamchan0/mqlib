/*****************************************************************************
    Todo: - Add source file description


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "InstrumentDelta.h"
#include "RiskInstrument.h"
#include "CachedDerivInstrument.h"
#include "CashInstrument.h"
#include "ForwardRateAgreement.h"
#include "Futures.h"
#include "ILZCSwapInstrument.h"
#include "InterestRateSwap.h"
#include "CrossCurrencySwap.h"
#include "TenorBasisSwap.h"
#include "OvernightIndexedSwap.h"
#include "OISBasisSwap.h"
#include "FixedRateBond.h"
#include "ZeroRate.h"
#include "ForwardZeroRate.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"

//	LTQuantLib
#include "Data/GenericData.h"


LTQC_ENUM_DEFINE_BEGIN( FlexYCF::DeltaType )
	LTQC_REGISTER_ENUM(	Delta				, "Delta"				);
	LTQC_REGISTER_ENUM(	TurnDelta			, "TurnDelta"			);
	LTQC_REGISTER_ENUM(	XCcySpreadDelta		, "XCcySpreadDelta"		);
	LTQC_REGISTER_ENUM(	TenorSpreadDelta	, "TenorSpreadDelta"	);
    LTQC_REGISTER_ENUM(	ILDelta	            , "ILDelta"	            );
	//	LTQC_REGISTER_ENUM(	, "" )
LTQC_ENUM_DEFINE_END( FlexYCF::DeltaType )



using namespace LTQC;

namespace FlexYCF
{
    InstrumentDelta::InstrumentDelta(const RiskInstrument& riskInstrument,
									 const double deltaValue,
                                     const double hedgeRatio,
									 const DeltaType deltaType):
		m_instrumentName(riskInstrument.getName()),
		m_instrumentDescription(riskInstrument.getDescription()),
		m_instrumentRate(riskInstrument.getRate()),
		m_deltaValue(deltaValue),
        m_hedgeRatio(hedgeRatio),
		m_deltaType(deltaType)
	{
	}

	DeltaType getDeltaType(const CachedDerivInstrument& instrument)
	{
		if(is_of_type<CashInstrument>(instrument) || is_of_type<ForwardRateAgreement>(instrument) || is_of_type<Futures>(instrument) || is_of_type<InterestRateSwap>(instrument) || is_of_type<FixedRateBondInstrument>(instrument))
		{
			return DeltaType::Delta;
		}
		if(is_of_type<CrossCurrencySwap>(instrument) || is_of_type<OvernightIndexedSwap>(instrument) || is_of_type<OISBasisSwap>(instrument) || is_of_type<ZeroRate>(instrument) || is_of_type<ForwardZeroRate>(instrument))
		{
			return DeltaType::XCcySpreadDelta;
		}
		if(is_of_type<TenorBasisSwap>(instrument))
		{
			return DeltaType::TenorSpreadDelta;
		}
        if(is_of_type<ILZCSwapInstrument>(instrument))
        {
            return DeltaType::ILDelta;
        }
		LTQC_THROW( LTQC::ModelQCException, "Cannot determine delta type: instrument with unknown type.");
	}

	InstrumentDeltaVector filterFromFullDeltaVector(const InstrumentDeltaVector& fullDeltaVector,
													const DeltaType deltaType)
	{
		InstrumentDeltaVector filteredDeltaVector;

		//	Filter using the helper predicate functor
		FlexYCF::details::InstrumentDeltaFilter deltaFilter(deltaType);
		FlexYCF::filter(fullDeltaVector, deltaFilter, filteredDeltaVector);

		return filteredDeltaVector;
	}

	void toGenericData(const InstrumentDeltaVector& deltaVector,
					   LTQuant::GenericData& instrumentDeltaData,
                       std::function<const double (const size_t)> instrumentRateProvider)
	{
		for(size_t i(0); i < deltaVector.size(); ++i)
		{			
			IDeA::inject<std::string>(instrumentDeltaData, IDeA_KEY(YC_DELTA, INSTRUMENT), i, deltaVector[i].getInstrumentName().string());
			IDeA::inject<std::string>(instrumentDeltaData, IDeA_KEY(YC_DELTA, MATURITY), i, deltaVector[i].getInstrumentDescription().string());
            IDeA::inject<double>(instrumentDeltaData, IDeA_KEY(YC_DELTA, RATE), i,instrumentRateProvider(i));
			IDeA::inject<double>(instrumentDeltaData, IDeA_KEY(YC_DELTA, DELTA), i, deltaVector[i].getDeltaValue());
            IDeA::inject<double>(instrumentDeltaData, IDeA_KEY(YC_DELTA, HEDGERATIO), i, deltaVector[i].getHedgeRatio());
		}
	}
	
	LT::TablePtr toTable(const InstrumentDeltaVector& deltaVector,std::function<const double (const size_t)> instrumentRateProvider)
	{
		LTQuant::GenericData genericData("Delta Vector", 0);

		toGenericData(deltaVector, genericData,instrumentRateProvider);

		return LTQuant::convertFromGenericData(FlexYCF::toSharedPtr(genericData))->table;
	}

    LT::TablePtr toTable(const InstrumentDeltaVector& deltaVector)
    {
        return toTable(deltaVector,[&deltaVector](const size_t i) ->const double {return  deltaVector[i].getInstrumentRate();});
    }
	
	std::vector<double> toVector(LT::TablePtr riskTbl, const LT::Str& key)
	{
		std::vector<double> result;
		if(riskTbl)
		{
			size_t r = riskTbl->rowsGet();
			// LT::Str deltaKey = IDeA_KEY(YC_DELTA, DELTA).getName();
			size_t col = riskTbl->findColKey( LT::NoThrow, key);

			if(col != LT::Table::not_found)
			{
				for( size_t i = 1; i < r; ++i)
				{
					result.push_back(riskTbl->at(i,col));
				}
			}
		}
		return result;
	}

	void mergeInstrumentDeltas(InstrumentDeltaVector& deltaVector1, const InstrumentDeltaVector& deltaVector2)
	{
		if(deltaVector1.empty())
		{
			deltaVector1 = deltaVector2;
			return;
		}

		if( deltaVector2.size() < deltaVector1.size() )
		{
			LTQC_THROW( LTQC::ModelQCException, "Cannot merge instrument deltas.");
		}
		
		for(size_t i = 0; i < deltaVector1.size(); ++i)
		{			
			deltaVector1[i].add(deltaVector2[i]);
		}
	}
}
