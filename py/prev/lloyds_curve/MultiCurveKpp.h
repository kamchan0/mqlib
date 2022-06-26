/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_MULTICURVEKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MULTICURVEKPP_H_INCLUDED
#pragma once
#include "BaseKnotPointPlacement.h"


namespace FlexYCF
{
    /// MultiCurveKpp is an abstract class that any knot-point
    /// placement that makes sense for all MultiCurveModel's
    /// should implement.
    class MultiCurveKpp : public BaseKnotPointPlacement
    {
            
    };

    DECLARE_SMART_PTRS( MultiCurveKpp )
}
#endif //__LIBRARY_PRICERS_FLEXYCF_MULTICURVEKPP_H_INCLUDED