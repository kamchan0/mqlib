/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ICurveFactory.h"
#include "BaseCurve.h"
#include "Data\GenericData.h"
#include "UkpCurve.h"
#include "ExtrapolationMethods.h"
#include "FlatRightInterpolation.h"


using namespace LTQC;

namespace FlexYCF
{
    ICurveFactory::ICurveFactory()
    {
        registerICurve<BaseCurve>();
    }


    ICurvePtr ICurveFactory::createInstance(const string& iCurveTypeName,
                                            const LTQuant::GenericDataPtr interpolationDetailsTable,
                                            const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
        return ICurveFactory::create(iCurveTypeName)(interpolationDetailsTable, leastSquaresResiduals);
    }

    ICurvePtr ICurveFactory::createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable,
                                            const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
        static const string defaultICurveTypeName(BaseCurve::getName());
        string iCurveTypeName(defaultICurveTypeName);

        if(static_cast<bool>(interpolationDetailsTable))
        {
            interpolationDetailsTable->permissive_get<string>("Curve Type", 0, iCurveTypeName, defaultICurveTypeName);
        }

        return ICurveFactory::createInstance(iCurveTypeName, interpolationDetailsTable, leastSquaresResiduals);
    }
        
    ICurvePtr ICurveFactory::createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable)
    {
        return createInstance(interpolationDetailsTable, LeastSquaresResidualsPtr());
    }

    ICurvePtr ICurveFactory::createFlatCurve(const double y)
    {
        const LTQuant::GenericDataPtr interpolationDetailsTable(new LTQuant::GenericData("", 0));
        const LTQuant::GenericDataPtr interpolationCurveDetailsTable(new LTQuant::GenericData("Interp Curve", 0));

        const LeastSquaresResidualsPtr leastSquaresResiduals;
        
        interpolationDetailsTable->set<LTQuant::GenericDataPtr>("Interp Curve", 0, interpolationCurveDetailsTable);

        interpolationCurveDetailsTable->set("Curve Type", 0, UkpCurve::getName());
        interpolationCurveDetailsTable->set("Interp", 0, FlatRightInterpolation::getName());
        interpolationDetailsTable->set("Left Extrap", 0, LeftFlatExtrapolationMethod::getName());
        interpolationDetailsTable->set("Right Extrap", 0, RightFlatExtrapolationMethod::getName());

        ICurvePtr flatCurve(BaseCurve::createInstance(interpolationDetailsTable, leastSquaresResiduals));

        flatCurve->addKnotPoint(KnotPoint(0.0, y, true));

        flatCurve->finalize();

        return flatCurve;
    }

    ICurveFactory * const ICurveFactory::instance()
    {
        static ICurveFactory iCurveFactory;
        return &iCurveFactory;
    }

}