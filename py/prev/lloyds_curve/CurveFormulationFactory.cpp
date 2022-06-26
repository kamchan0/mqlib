#include "stdafx.h"
#include "CurveFormulationFactory.h"
#include "DiscountCurve.h"
#include "InstantaneousForwardCurve.h"
#include "MinusLogDiscountCurve.h"
#include "SpotRateCurve.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    CurveFormulationFactory::CurveFormulationFactory()
    {
        registerCurveFormulation<DiscountCurve>();
        registerCurveFormulation<InstantaneousForwardCurve>();
        registerCurveFormulation<MinusLogDiscountCurve>();
        registerCurveFormulation<SpotRateCurve>();
    }

    CurveFormulationPtr CurveFormulationFactory::createInstance(const string& curveFormulationName,
                                                                const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                                const string& curveDescription,
                                                                const LeastSquaresResidualsPtr& leastSquaresResiduals)
    {
        return CurveFormulationFactory::create(curveFormulationName)(interpolationDetailsTable, curveDescription, leastSquaresResiduals);
    }

    CurveFormulationFactory* const CurveFormulationFactory::instance()
    {
        static CurveFormulationFactory curveFormulationFactory;
        return &curveFormulationFactory;
    }

    
}