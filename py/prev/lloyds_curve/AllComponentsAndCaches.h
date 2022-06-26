/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_ALLCOMPONENTSANDCACHES_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_ALLCOMPONENTSANDCACHES_H_INCLUDED
#pragma once

//  This header includes all the instrument components arguments headers
//  and typedefine the concrete instrument components and their caches from them

#include "InstrumentComponentCache.h"
#include "GenericInstrumentComponent.h"
#include "DiscountFactorArguments.h"
#include "TenorDiscountFactorArguments.h"
#include "ForwardRateArguments.h"
#include "DiscountedForwardRateArguments.h"
#include "FixedLegArguments.h"
#include "FloatingLegArguments.h"
#include "FixedCashFlowArguments.h"

#include "DiscountFactor.h"
#include "TenorDiscountFactor.h"
#include "ForwardRate.h"
#include "DiscountedForwardRate.h"
#include "FixedLeg.h"
#include "FloatingLeg.h"
#include "FixedCashFlow.h"
#include "InflationIndex.h"

namespace FlexYCF
{
    //  Discount Factor
    typedef InstrumentComponentCache<DiscountFactorArguments>           DiscountFactorCache;
    DECLARE_SMART_PTRS( DiscountFactorCache )
    
    //  Tenor Discount Factor
    typedef InstrumentComponentCache<TenorDiscountFactorArguments>      TenorDiscountFactorCache;
    DECLARE_SMART_PTRS( TenorDiscountFactorCache )
  
    //  Forward Rate
    typedef InstrumentComponentCache<ForwardRateArguments, ForwardRate> ForwardRateCache;
    DECLARE_SMART_PTRS( ForwardRateCache )
    
    //  Discounted Forward Rate
    typedef InstrumentComponentCache< DiscountedForwardRateArguments, DiscountedForwardRate > DiscountedForwardRateCache;
    DECLARE_SMART_PTRS( DiscountedForwardRateCache )
    
    // Fixed Leg
    typedef InstrumentComponentCache<FixedLegArguments, FixedLeg>       FixedLegCache;
    DECLARE_SMART_PTRS( FixedLegCache )

    //  Floating Leg
    //typedef GenericInstrumentComponent<FloatingLegArguments>    FloatingLeg;
    DECLARE_SMART_PTRS( FloatingLeg )
    typedef InstrumentComponentCache<FloatingLegArguments, FloatingLeg> FloatingLegCache;
    DECLARE_SMART_PTRS( FloatingLegCache )

    //  Fixed Cash Flow
    typedef InstrumentComponentCache< FixedCashFlowArguments,
                                      FixedCashFlow > FixedCashFlowCache;

    DECLARE_SMART_PTRS( FixedCashFlowCache )

	//  Inflation Index
	typedef InstrumentComponentCache<InflationIndexArguments>	InflationIndexCache;
	DECLARE_SMART_PTRS( InflationIndexCache )
   
}
#endif //__LIBRARY_PRICERS_FLEXYCF_ALLCOMPONENTSANDCACHES_H_INCLUDED