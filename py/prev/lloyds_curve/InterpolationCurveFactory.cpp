#include "stdafx.h"
#include "InterpolationCurveFactory.h"
#include "UkpCurve.h"
#include "TensionSpline.h"
#include "CompositeCurve.h"
#include "Data\GenericData.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    InterpolationCurveFactory::InterpolationCurveFactory()
    {
        registerInterpolationCurve<UkpCurve>();
        registerInterpolationCurve<TensionSpline>();
        registerInterpolationCurve<CompositeCurve>();
    }

    InterpolationCurveFactory* const InterpolationCurveFactory::instance()
    {
        static InterpolationCurveFactory interpolationCurveFactory;
        return &interpolationCurveFactory;
    }

    InterpolationCurvePtr InterpolationCurveFactory::createInstance(const string& interpolationCurveTypeName,
                                                                    const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                                    const KnotPointsPtr knotPoints,
                                                                    const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
        return InterpolationCurveFactory::create(interpolationCurveTypeName)(interpolationCurveDetailsTable, knotPoints, leastSquaresResiduals);
    }

   InterpolationCurvePtr InterpolationCurveFactory::createInstance(const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                                   const KnotPointsPtr knotPoints,
                                                                   const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
        static const string defaultInterpolationCurveTypeName(UkpCurve::getName());
        string interpolationCurveTypeName(defaultInterpolationCurveTypeName);

        if(static_cast<bool>(interpolationCurveDetailsTable))
        {
            interpolationCurveDetailsTable->permissive_get<string>("Curve Type", 0, interpolationCurveTypeName, defaultInterpolationCurveTypeName);
        }
        
        return InterpolationCurveFactory::createInstance(interpolationCurveTypeName, interpolationCurveDetailsTable, knotPoints, leastSquaresResiduals);

    }

}