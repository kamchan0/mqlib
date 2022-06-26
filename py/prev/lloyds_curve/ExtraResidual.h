/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EXTRARESIDUAL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXTRARESIDUAL_H_INCLUDED
#pragma once
#include "WeightedResidual.h"


namespace FlexYCF
{

    // This class may not be needed, any concrete extra residual
    // can inherit directly from WeightedResidual instead, although
    //  in this case it would then be possible for ExtraResiduals to
    // include InstrumentResidual's
    class ExtraResidual : public WeightedResidual
    {
    public:
        explicit ExtraResidual(const double weight = 1.0):
            WeightedResidual(weight)
        {
        }

		virtual void computeGradient(Gradient& gradien) const = 0;
		
		virtual void computeGradient(Gradient& gradient, 
									 const CurveTypeConstPtr& /* curveType */) const
		{
			// By default, does not support extra residuals gradient calculations 
			// relative to a specific curve type
			computeGradient(gradient);
		}

    };  //  ExtraResidual

    DECLARE_SMART_PTRS( ExtraResidual )

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_EXTRARESIDUAL_H_INCLUDED