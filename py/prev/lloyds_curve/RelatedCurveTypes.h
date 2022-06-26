/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_RELATEDCURVETYPES_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_RELATEDCURVETYPES_H_INCLUDED
#pragma once

#include "CurveType.h"
#include "DateUtils.h"


namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( CalibrationInstrument )
    FWD_DECLARE_SMART_PTRS( CashInstrument )
    FWD_DECLARE_SMART_PTRS( ForwardRateAgreement )
    FWD_DECLARE_SMART_PTRS( Futures)
    FWD_DECLARE_SMART_PTRS( CrossCurrencySwap )
    FWD_DECLARE_SMART_PTRS( InterestRateSwap )
	FWD_DECLARE_SMART_PTRS( OvernightIndexedSwap )
    FWD_DECLARE_SMART_PTRS( OISBasisSwap )
    FWD_DECLARE_SMART_PTRS( TenorBasisSwap )
    FWD_DECLARE_SMART_PTRS( ZeroRate )
    FWD_DECLARE_SMART_PTRS( TenorZeroRate )
	FWD_DECLARE_SMART_PTRS( ForwardZeroRate )

    /// RelatedCurveTypes is a class that encapsulates static
    /// functions that return the types of the curves each 
    /// calibration instrument contains the most information on.
    class RelatedCurveTypes
    {
    public:
        static CurveTypes get(const CalibrationInstrumentPtr calibrationInstrument);
        static CurveTypes get(const CashInstrumentPtr cashInstrument);
        static CurveTypes get(const ForwardRateAgreementPtr fra);
        static CurveTypes get(const FuturesPtr futures);
        static CurveTypes get(const CrossCurrencySwapPtr crossCurrencySwap);
        static CurveTypes get(const InterestRateSwapPtr interestRateSwap);
        static CurveTypes get(const TenorBasisSwapPtr tenorBasisSwap);
		static CurveTypes get(const OvernightIndexedSwapPtr ois);
        static CurveTypes get(const OISBasisSwapPtr ois);
		static CurveTypes get(const ZeroRatePtr z);
        static CurveTypes get(const TenorZeroRatePtr z);
    
    };  // RelatedCurveTypes

}   // FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_RELATEDCURVETYPES_H_INCLUDED