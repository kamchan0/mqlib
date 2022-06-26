/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __COMPOSITE_CURVE_SEPARATION_UTILS__
#define __COMPOSITE_CURVE_SEPARATION_UTILS__
#pragma once

#include "LTQuantInitial.h"


namespace FlexYCF
{
	FWD_DECLARE_SMART_PTRS( BaseCurve );
	FWD_DECLARE_SMART_PTRS( CurveFormulation );

	// Try to set the separation point of the specified curve at the
	//	maturity of the last future
	//
	//	Note:
	//	If the if the BaseCurve specified does not contain a composite curve
	//	as an interpolation curve, or if the knot-points in the curve are not
	//	related to at least one future, or if the separation knot has been
	//	explicitly set already (i.e. the default value, whather this may be, 
	//	is used), the function has *NO* effect on the curve
	void setSeparationPoint(const LT::date valueDate, const BaseCurve& baseCurve);

	//	Curve formulation version (for StripperModel)
	//	just delegating the job to its inner base curve
	void setSeparationPoint(const LT::date valueDate, const CurveFormulation& curveFormulation);
}
#endif	//	__COMPOSITE_CURVE_SEPARATION_UTILS__