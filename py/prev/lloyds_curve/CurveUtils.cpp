/*****************************************************************************
    CurveUtils

	Implementation of FlexYCF curve utility functions


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2010 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "CurveUtils.h"
#include "ICurve.h"
#include "KnotPoint.h"


using namespace LTQC;

namespace FlexYCF
{

	void finalizeLogFvfCurve(ICurve& curve)
	{
		//	Always add the origin if less than 2 knot-points
		//	Note: an exception is thrown if a knot-point
		//	is already at located at x = 0.0
		curve.addKnotPoint(KnotPoint::origin());
		
		if(curve.getNumberOfKnots() == 1)
		{
			//	Log-fvf curves have a flat extrapolation on the left 
			//	and a straight line extrapolation on the right. 
			//	Therefore, they must always have at least two knot-points
			//	to build properly without modifying the settings.
			curve.addKnotPoint(KnotPoint::xNegativeHelper());
		}

		//	Now finalize
		curve.finalize();
	}

}