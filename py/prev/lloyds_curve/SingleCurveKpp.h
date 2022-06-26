/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEKPP_H_INCLUDED
#pragma once

#include "BaseKnotPointPlacement.h"

namespace FlexYCF
{
    /// SingleCurveKpp is an abstract class that any knot-point
    /// placement that makes sense for all SingleCurveModel's
    /// should implement.
    class SingleCurveKpp : public BaseKnotPointPlacement
    {
        // probably implements createKnotPoints, doing the dynamic down-cast to a SingleCurveModel (shared?) pointer,
        // an exposes the same interface with a pointer to SingleCurveModel
    };

    DECLARE_SMART_PTRS( SingleCurveKpp )
}
#endif //__LIBRARY_PRICERS_FLEXYCF_SINGLECURVEKPP_H_INCLUDED