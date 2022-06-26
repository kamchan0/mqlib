/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

#include "stdafx.h"
#include "InterpolationMethodFactory.h"
#include "BiQuadraticInterpolation.h"
#include "CubicSplineInterpolation.h"
#include "FlatLeftInterpolation.h"
#include "FlatRightInterpolation.h"
#include "MonotoneConvexSplineInterpolation.h"
#include "StraightLineInterpolation.h"

using namespace std;

namespace FlexYCF
{
    InterpolationMethodFactory::InterpolationMethodFactory()
    {
        registerInterpolation<BiQuadraticInterpolation>();

        registerObject("CubicSpline", InterpolationMethodPtr(new CubicSplineInterpolation));
        
        registerInterpolation<FlatLeftInterpolation>();
        registerInterpolation<FlatRightInterpolation>();

        registerInterpolation<MonotoneConvexSplineInterpolation>();
        registerObject("MCS", InterpolationMethodPtr(new MonotoneConvexSplineInterpolation));
        
        registerInterpolation<StraightLineInterpolation>();
    }

    InterpolationMethodPtr InterpolationMethodFactory::create(const string& interpolationMethodName)
    {
        InterpolationMethodPtr internalInstance(instance()->internalCreate(interpolationMethodName));
        return internalInstance->clone();
    }

    InterpolationMethodFactory* InterpolationMethodFactory::instance()
    {
        static InterpolationMethodFactory theInstance;
        return &theInstance;
    }
}