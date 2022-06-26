/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "TssInterpolationFactory.h"
#include "BasicTssInterpolation.h"
#include "BucketedTssInterpolation.h"
#include "TolerantStraightLineTssInterpolation.h"

#include "Data/GenericData.h"


using namespace LTQC;

namespace FlexYCF
{
	BaseTssInterpolationPtr TssInterpolationFactory::createTssInterpolation(const std::string& tssInterpolationName,
																						const CurveTypeConstPtr& baseRate,
																						TenorSpreadSurface::TypedCurves& tenorCurves,
																						const LTQuant::GenericDataPtr& tssInterpParams)
	{
		return TssInterpolationFactory::create(tssInterpolationName)(baseRate, tenorCurves, tssInterpParams);
	}

	TssInterpolationFactory* TssInterpolationFactory::instance()
	{
		static TssInterpolationFactory theTssInterpolationFactory;
		return &theTssInterpolationFactory;
	}
	
	TssInterpolationFactory::TssInterpolationFactory()
	{
		registerTssInterpolation<BasicTssInterpolation>();
		registerTssInterpolation<BucketedTssInterpolation>();
		registerTssInterpolation<TolerantStraightLineTssInterpolation>();
	}
}