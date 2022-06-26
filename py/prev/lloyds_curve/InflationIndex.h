/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INFLATIONINDEX_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INFLATIONINDEX_H_INCLUDED
#pragma once

#include "InflationIndexArguments.h"
#include "GenericInstrumentComponent.h"


namespace FlexYCF
{
    typedef GenericInstrumentComponent<InflationIndexArguments> InflationIndex;
	typedef std::tr1::shared_ptr<InflationIndex> InflationIndexPtr;
	size_t hash_value(const InflationIndexPtr inflationIndex);
}
#endif //__LIBRARY_PRICERS_FLEXYCF_INFLATIONINDEX_H_INCLUDED