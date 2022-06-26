/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BASEEXTRAPOLATIONMETHODS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BASEEXTRAPOLATIONMETHODS_H_INCLUDED
#pragma once

#include "ExtrapolationSpecs.h"
#include "GenericExtrapolationMethod.h"


namespace FlexYCF
{
    /// Contains declarations of the base classes for the left
    /// and right extrapolation methods hierarchies.
    typedef GenericExtrapolationMethod<LeftExtrapSpec>      LeftExtrapolation;    
    typedef GenericExtrapolationMethod<RightExtrapSpec>     RightExtrapolation;

    DECLARE_SMART_PTRS( LeftExtrapolation  )
    DECLARE_SMART_PTRS( RightExtrapolation )
}

#endif //__LIBRARY_PRICERS_FLEXYCF_BASEEXTRAPOLATIONMETHODS_H_INCLUDED