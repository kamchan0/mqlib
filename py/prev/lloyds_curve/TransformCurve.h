/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TRANSFORMCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TRANSFORMCURVE_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "BaseCurve.h"
#include "Interpolation.h"
#include "TransformFunction.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )

    // TO DO: - Add a CurveIntegral decorator of BaseCurve with the same interface

    /// Represents a curve that is the tranform of an original curve
    /// by a transform function.
    ///
    class TransformCurve : public BaseCurve
    {
    public:
        // explicit TransformCurve(const InterpolationMethodPtr& interpolationMethod, const TransformFunctionPtr& transformFunction);
       // TransformCurve(const InterpolationCurvePtr interpolationCurve, 
        //               const TransformFunctionPtr transformFunction);
        
        TransformCurve(const KnotPointsPtr knotPoints,
                       const InterpolationCurvePtr interpolationCurve, 
                       const TransformFunctionPtr transformFunction);

        TransformCurve(const KnotPointsPtr knotPoints,
                       const InterpolationCurvePtr interpolationCurve,
                       const LeftExtrapolationPtr leftExtrapolationMethod,
                       const RightExtrapolationPtr rightExtrapolationMethod,
                       const TransformFunctionPtr transformFunction);

        static std::string getName();

        /// Adds a knot point to the TransformCurve, assuming the y value of the knot-point is the 
        /// transformed value (by the TransformFunction::doTransform) of an 'original' y value
        virtual void addKnotPoint(const KnotPoint& knotPoint);
        
        static ICurvePtr createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable,
                                        const LeastSquaresResidualsPtr leastSquaresResiduals);

        virtual double evaluate(const double x) const;
        virtual void accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const; 

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        TransformCurve(TransformCurve const& original, CloneLookup& lookup); 

    private:
        TransformCurve(TransformCurve const&); // deliberately disabled as won't clone properly

        TransformFunctionPtr m_transformFunction;
    };

    DECLARE_SMART_PTRS(TransformCurve)
}
#endif //__LIBRARY_PRICERS_FLEXYCF_TRANSFORMCURVE_H_INCLUDED