/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "BaseTssInterpolation.h"
#include "ICurve.h"

using namespace LTQC;

namespace FlexYCF
{
	//	Default general implementation
	void BaseTssInterpolation::accumulateGradient(const double tenor,  
												  const double flowTime,
												  double multiplier, 
												  GradientIterator gradientBegin,
												  GradientIterator gradientEnd,
												  const CurveTypeConstPtr& curveType) const
	{
		if(CurveType::getFromYearFraction(tenor) != baseRate())
        {
			if(curveType == CurveType::AllTenors())
			{
				accumulateGradient(tenor, flowTime, multiplier, gradientBegin, gradientEnd);
			}
			else
			{
				// New: shorter gradient, ignoring the (zero) sensitivities to other curves
				TypedCurves::const_iterator iter(curves().lowerBound(curveType));
				if(iter != curves().end() && iter->first == curveType)
				{
					iter->second->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
				}
			}
		}
		
	}
	
	LTQuant::GenericDataPtr BaseTssInterpolation::TSSInterpParameters() const 
	{ 
		return  LTQuant::GenericDataPtr(); 
	}
}	//	FlexYCF