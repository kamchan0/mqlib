/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODS_H_INCLUDED
#pragma once

#include "BaseExtrapolationMethods.h"
#include "FlatExtrapolationMethod.h"
#include "StraightLineExtrapolationMethod.h"


namespace FlexYCF
{
    typedef FlatExtrapolationMethod<LeftExtrapSpec>             LeftFlatExtrapolationMethod;
    typedef FlatExtrapolationMethod<RightExtrapSpec>            RightFlatExtrapolationMethod;
    typedef StraightLineExtrapolationMethod<LeftExtrapSpec>     LeftStraightLineExtrapolationMethod;
    typedef StraightLineExtrapolationMethod<RightExtrapSpec>    RightStraightLineExtrapolationMethod;

    DECLARE_SMART_PTRS( LeftFlatExtrapolationMethod )
    DECLARE_SMART_PTRS( RightFlatExtrapolationMethod )
    DECLARE_SMART_PTRS( LeftStraightLineExtrapolationMethod )
    DECLARE_SMART_PTRS( RightStraightLineExtrapolationMethod )
}

#endif //__LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODS_H_INCLUDED