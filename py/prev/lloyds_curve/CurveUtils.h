/*****************************************************************************

	CurveUtils

	Utility functions for FlexYCF curve

    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2010 All Rights Reserved

*****************************************************************************/
#ifndef __CurveUtils_H__
#define __CurveUtils_H__


namespace FlexYCF
{
	class ICurve;

	//	Finalizes a curve represented in the "Log-fvf" formulation
	void finalizeLogFvfCurve(ICurve& curve);

}
#endif	//	__CurveUtils_H__