/*****************************************************************************
    Todo: - Add header file description


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/

#ifndef __LIBRARY_FLEXYCF_CURVECREATIONHELPER_H_INCLUDED
#define __LIBRARY_FLEXYCF_CURVECREATIONHELPER_H_INCLUDED
#pragma once

#include "BaseCurve.h"
#include "KnotPoints.h"
#include "UkpCurve.h"
#include "InterpolationMethodFactory.h"
#include "ExtrapolationMethodFactoryDefs.h"

namespace FlexYCF
{

    template<class Interp,
             class LeftExtrap,
             class RightExtrap>
    static BaseCurvePtr createUkpCurve()
    {
        KnotPointsPtr knotPoints(new KnotPoints);
        InterpolationCurvePtr interpCurve(new UkpCurve(knotPoints, InterpolationMethodFactory::create(Interp::getName())));
        LeftExtrapolationPtr leftExtrapolation(
            LeftExtrapolationMethodFactory::createInstance(
			    LeftExtrap::getName(),
				[interpCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd) 
						{return interpCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}
			)
		);
        RightExtrapolationPtr rightExtrapolation(
            RightExtrapolationMethodFactory::createInstance(
				RightExtrap::getName(),
				[interpCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd) 
						{return interpCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}
			)
		);
         return BaseCurvePtr(new BaseCurve(knotPoints, interpCurve, leftExtrapolation, rightExtrapolation));
        
    }
}

#endif __LIBRARY_FLEXYCF_CURVECREATIONHELPER_H_INCLUDED