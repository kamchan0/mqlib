/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DISCOUNTFACTOR2_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DISCOUNTFACTOR2_H_INCLUDED
#pragma once

#include "DiscountFactorArguments.h"
#include "GenericInstrumentComponent.h"


namespace FlexYCF
{
    typedef GenericInstrumentComponent<DiscountFactorArguments> DiscountFactor;
	typedef std::tr1::shared_ptr<DiscountFactor> DiscountFactorPtr;
	size_t hash_value(const DiscountFactorPtr discountFactor);
}
#endif //__LIBRARY_PRICERS_FLEXYCF_DISCOUNTFACTOR2_H_INCLUDED