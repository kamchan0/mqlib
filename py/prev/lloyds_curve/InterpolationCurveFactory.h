/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONCURVEFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONCURVEFACTORY_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"

// DevCore
#include "DevCore/NameFactory.h"

#include <functional>

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS;
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( InterpolationCurve );
    FWD_DECLARE_SMART_PTRS( KnotPoints );
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals );

	typedef std::tr1::function <InterpolationCurvePtr
                              (const LTQuant::GenericDataPtr,
                              const KnotPointsPtr,
                              const LeastSquaresResidualsPtr)> InterpolationCurveCreationFunction;

    class InterpolationCurveFactory : public DevCore::NameFactory <InterpolationCurveCreationFunction,
                                                                   InterpolationCurveFactory,
                                                                   InterpolationCurveCreationFunction>
    {
    public:
        static InterpolationCurvePtr createInstance(const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                    const KnotPointsPtr knotPoints,
                                                    const LeastSquaresResidualsPtr leastSquaresResiduals);

        static InterpolationCurvePtr createInstance(const std::string& interpolationCurveTypeName,
                                                    const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                    const KnotPointsPtr knotPoints,
                                                    const LeastSquaresResidualsPtr leastSquaresResiduals);
        
    private:
		friend class DevCore::NameFactory <InterpolationCurveCreationFunction,
                                           InterpolationCurveFactory,
                                           InterpolationCurveCreationFunction>;
        
		static InterpolationCurveFactory* const instance();

		explicit InterpolationCurveFactory();

        template <class CIC>
        static void registerInterpolationCurve()
        {
            InterpolationCurveFactory::instance()->registerObject(CIC::getName(), CIC::createInstance);
        }
    };
                
}

#endif //__LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONCURVEFACTORY_H_INCLUDED