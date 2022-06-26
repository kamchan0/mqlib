/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TENORDISCOUNTFACTOR_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TENORDISCOUNTFACTOR_H_INCLUDED
#pragma once

#include "GenericInstrumentComponent.h"
#include "TenorDiscountFactorArguments.h"

namespace FlexYCF
{
    typedef GenericInstrumentComponent<TenorDiscountFactorArguments>    TenorDiscountFactor;
    DECLARE_SMART_PTRS( TenorDiscountFactor )
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_TENORDISCOUNTFACTOR_H_INCLUDED