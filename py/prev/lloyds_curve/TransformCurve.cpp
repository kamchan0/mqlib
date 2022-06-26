/*****************************************************************************
    
	Implementation of TransformCurve

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "TransformCurve.h"
#include "InterpolationMethod.h"
#include "UkpCurve.h"
#include "TransformFunctionFactory.h"
#include "ExtrapolationMethods.h"
#include "InterpolationCurveFactory.h"
#include "ExtrapolationMethodFactoryDefs.h"
#include "GenericData.h"

//	LTQuantLib

using namespace LTQC;

namespace FlexYCF
{
    TransformCurve::TransformCurve(const KnotPointsPtr knotPoints,
                                   const InterpolationCurvePtr interpolationCurve, 
                                   const TransformFunctionPtr transformFunction):
        BaseCurve(knotPoints,
                  interpolationCurve,
				  LeftExtrapolationMethodFactory::createInstance("Flat", 
					[interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd) 
						{return interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}),
				  RightExtrapolationMethodFactory::createInstance("Flat", 
					[interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd) 
						{return interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);})),
                  m_transformFunction(transformFunction)
    {
    }

    TransformCurve::TransformCurve(const KnotPointsPtr knotPoints,
                                   const InterpolationCurvePtr interpolationCurve,
                                   const LeftExtrapolationPtr leftExtrapolationMethod,
                                   const RightExtrapolationPtr rightExtrapolationMethod,
                                   const TransformFunctionPtr transformFunction):
        BaseCurve(knotPoints,
                  interpolationCurve,
                  leftExtrapolationMethod,
                  rightExtrapolationMethod),
                  m_transformFunction(transformFunction)
    {
    }

    string TransformCurve::getName()
    {
        return "Transform";
    }
        
    ICurvePtr TransformCurve::createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable,
                                             const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
        // Note : This is essentially a copy paste from BaseCurve + TransformFunction
        const string defaultInterpolationCurveTypeName(UkpCurve::getName());
        const string defaultLeftExtrapolationMethodName(LeftFlatExtrapolationMethod::getName());
        const string defaultRightExtrapolationMethodName(RightStraightLineExtrapolationMethod::getName());

        // Create KnotPoints object
        const KnotPointsPtr knotPoints(new KnotPoints);

        // Create InterpolationCurve from table
        LTQuant::GenericDataPtr interpolationCurveDetailsTable;
        if(static_cast<bool>(interpolationDetailsTable))
        {
            interpolationDetailsTable->permissive_get<LTQuant::GenericDataPtr>("Interp Curve", 0, interpolationCurveDetailsTable, LTQuant::GenericDataPtr());
        }

        string interpolationCurveTypeName(defaultInterpolationCurveTypeName);
        if(static_cast<bool>(interpolationCurveDetailsTable))
        {
            interpolationCurveDetailsTable->permissive_get<string>("Curve Type", 0, interpolationCurveTypeName, defaultInterpolationCurveTypeName);
        }

        InterpolationCurvePtr interpolationCurve(InterpolationCurveFactory::createInstance(interpolationCurveTypeName, 
                                                                                           interpolationCurveDetailsTable,
                                                                                           knotPoints,
                                                                                           leastSquaresResiduals));

        // Create LeftExtrapolationMethod
        string leftExtrapolationMethodName(defaultLeftExtrapolationMethodName);
        if(static_cast<bool>(interpolationCurveDetailsTable))
        {
            interpolationDetailsTable->permissive_get<string>("Left Extrap", 0, leftExtrapolationMethodName, defaultLeftExtrapolationMethodName);
        }
		LeftExtrapolationPtr leftExtrapolationMethod  (
			LeftExtrapolationMethodFactory::createInstance(
				leftExtrapolationMethodName, 
				[interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd)
				{return interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}
			)
		);

        // Create RigthExtrapolationMethod
        string rightExtrapolationMethodName(defaultRightExtrapolationMethodName);
        if(static_cast<bool>(interpolationCurveDetailsTable))
        {
            interpolationDetailsTable->permissive_get<string>("Right Extrap", 0, rightExtrapolationMethodName, defaultRightExtrapolationMethodName);
        }
		RightExtrapolationPtr rightExtrapolationMethod    (
			RightExtrapolationMethodFactory::createInstance(
				rightExtrapolationMethodName,
				[interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd)
				{return interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}
			)
		);
        // specific code for TransformFunction goes here... for now
        const string defaultTransformFunctionName("Null");
        string transformFunctionName(defaultTransformFunctionName);
        if(static_cast<bool>(interpolationDetailsTable))
        {
            interpolationDetailsTable->permissive_get<string>("Transform", 0, transformFunctionName, defaultTransformFunctionName);
        }

        // should be in a TransformFunctionFactory...
        TransformFunctionPtr transformFunction(TransformFunctionFactory::createInstance(transformFunctionName));

        return ICurvePtr(TransformCurvePtr(new TransformCurve(knotPoints,
                                                              interpolationCurve,
                                                              leftExtrapolationMethod,
                                                              rightExtrapolationMethod,
                                                              transformFunction)));
    }

    void TransformCurve::addKnotPoint(const KnotPoint& knotPoint)
    {
        KnotPoint newKnotPoint(knotPoint);
        newKnotPoint.y = m_transformFunction->doInverseTransform(newKnotPoint.y);
        BaseCurve::addKnotPoint(newKnotPoint);
    }

    double TransformCurve::evaluate(const double x) const
    { 
        double rawY(BaseCurve::evaluate(x));
        return m_transformFunction->doTransform(rawY);
    }

    void TransformCurve::accumulateGradient(const double x, 
                                            double multiplier,
                                            GradientIterator gradientBegin,
                                            GradientIterator gradientEnd) const
    { 
        double rawY(BaseCurve::evaluate(x));
        double transformDerivative(m_transformFunction->derivative(rawY));
        BaseCurve::accumulateGradient(x, multiplier * transformDerivative, gradientBegin, gradientEnd); 
    }

    /**
        @brief Clone this instance.

        Uses a lookup of previously created clones to ensure that directed graph relationships are maintained.

        @param lookup A lookup of previously created clones.
        @return       A clone.
    */
    ICloneLookupPtr TransformCurve::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new TransformCurve(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup of previously created clones to ensure that directed graph relationships are maintained. This 
        constructor assumes that the transform function does not have smart pointer to other objects in the FlexYCF curve.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    TransformCurve::TransformCurve(TransformCurve const& original, CloneLookup& lookup) : 
        BaseCurve(original, lookup),
        m_transformFunction(original.m_transformFunction->clone())
    {
    }
}   // FlexYCF
