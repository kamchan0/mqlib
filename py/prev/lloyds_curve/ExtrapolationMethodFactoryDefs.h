/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

/// \file

#ifndef __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODFACTORYDEFS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODFACTORYDEFS_H_INCLUDED
#pragma once

#include "ExtrapolationSpecs.h"
#include "ExtrapolationMethodFactory.h"

namespace FlexYCF
{
    // Left/Right[ExtrapolationMethod] Factories:
    typedef ExtrapolationMethodFactory<LeftExtrapSpec>  LeftExtrapolationMethodFactory;
    typedef ExtrapolationMethodFactory<RightExtrapSpec> RightExtrapolationMethodFactory;
}
#endif // __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODFACTORYDEFS_H_INCLUDED