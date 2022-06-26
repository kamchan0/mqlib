/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "RelatedCurveTypes.h"
#include "CalibrationInstrument.h"
#include "CashInstrument.h"
#include "CrossCurrencySwap.h"
#include "ForwardRateAgreement.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "OvernightIndexedSwap.h"
#include "OISBasisSwap.h"
#include "TenorBasisSwap.h"
#include "ZeroRate.h"
#include "TenorZeroRate.h"
#include "ForwardZeroRate.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    CurveTypes RelatedCurveTypes::get(const CalibrationInstrumentPtr calibrationInstrument)
    {
        //  CashInstrument
        if(isOfType<CashInstrument>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<CashInstrument>(calibrationInstrument));
        }
        //  FRA
        else if(isOfType<ForwardRateAgreement>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<ForwardRateAgreement>(calibrationInstrument));
        }
        //  Futures
        else if(isOfType<Futures>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<Futures>(calibrationInstrument));
        }
        // CrossCurrencySwap
        else if(isOfType<CrossCurrencySwap>(calibrationInstrument))
        {
            return get(std::tr1::dynamic_pointer_cast<CrossCurrencySwap>(calibrationInstrument));
        }
        // InterestRateSwap
        else if(isOfType<InterestRateSwap>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<InterestRateSwap>(calibrationInstrument));
        }
        //  TenorBasisSwap
        else if(isOfType<TenorBasisSwap>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<TenorBasisSwap>(calibrationInstrument));
        }
        else if(isOfType<OvernightIndexedSwap>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<OvernightIndexedSwap>(calibrationInstrument));
        }
        else if(isOfType<OISBasisSwap>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<OISBasisSwap>(calibrationInstrument));
        }
		else if(isOfType<ZeroRate>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<ZeroRate>(calibrationInstrument));
        }
		else if(isOfType<TenorZeroRate>(calibrationInstrument))
        {
			return get(std::tr1::dynamic_pointer_cast<TenorZeroRate>(calibrationInstrument));
        }
		else if(isOfType<ForwardZeroRate>(calibrationInstrument))
		{
			return get(std::tr1::dynamic_pointer_cast<ForwardZeroRate>(calibrationInstrument));
		}
        else

            throw exception("Instrument type not valid."); 
    }
    
    CurveTypes RelatedCurveTypes::get(const CashInstrumentPtr cashInstrument)
    {
        CurveTypes relatedCurveTypes;
    
        relatedCurveTypes.push_back(CurveType::getFromYearFraction(cashInstrument->getTenor()));

        return relatedCurveTypes;
    }

    CurveTypes RelatedCurveTypes::get(const FuturesPtr futures)
    {
        CurveTypes relatedCurveTypes;

        relatedCurveTypes.push_back(CurveType::_3M());

        return relatedCurveTypes;
    }

    CurveTypes RelatedCurveTypes::get(const ForwardRateAgreementPtr fra)
    {
        CurveTypes relatedCurveTypes;

        relatedCurveTypes.push_back( fra->getCurveType( ) );

        return relatedCurveTypes;
    }
    
    CurveTypes RelatedCurveTypes::get(const CrossCurrencySwapPtr crossCurrencySwap)
    {
        CurveTypes relatedCurveTypes;

        relatedCurveTypes.push_back(CurveType::Discount());
        relatedCurveTypes.push_back(CurveType::getFromYearFraction(crossCurrencySwap->getDomesticLegTenor()));
        
        return relatedCurveTypes;   
    }
   
    
    CurveTypes RelatedCurveTypes::get(const InterestRateSwapPtr interestRateSwap)
    {
        CurveTypes relatedCurveTypes;

        relatedCurveTypes.push_back(CurveType::getFromYearFraction(interestRateSwap->getFloatingLegTenor()));
        
        return relatedCurveTypes;        
    }

    CurveTypes RelatedCurveTypes::get(const TenorBasisSwapPtr tenorBasisSwap)
    {
        CurveTypes relatedCurveTypes;

        relatedCurveTypes.push_back(CurveType::getFromYearFraction(tenorBasisSwap->getShortTenor()));
        relatedCurveTypes.push_back(CurveType::getFromYearFraction(tenorBasisSwap->getLongTenor()));

        return relatedCurveTypes;
    }

	CurveTypes RelatedCurveTypes::get(const OvernightIndexedSwapPtr ois)
    {
        CurveTypes relatedCurveTypes;

        relatedCurveTypes.push_back(CurveType::Discount());
        relatedCurveTypes.push_back(CurveType::Discount());

        return relatedCurveTypes;   
    }
    
    CurveTypes RelatedCurveTypes::get(const OISBasisSwapPtr ois)
    {
        CurveTypes relatedCurveTypes;

        relatedCurveTypes.push_back(CurveType::Discount());
        relatedCurveTypes.push_back(CurveType::Discount());

        return relatedCurveTypes;   
    }

	CurveTypes RelatedCurveTypes::get(const ZeroRatePtr z)
    {
        CurveTypes relatedCurveTypes;
		relatedCurveTypes.push_back(CurveType::Discount());
        return relatedCurveTypes;   
    }

	CurveTypes RelatedCurveTypes::get(const TenorZeroRatePtr z)
    {
        CurveTypes relatedCurveTypes;
        relatedCurveTypes.push_back( z->getCurveType( ) );
        return relatedCurveTypes;
    }
}   //  FlexYCF