/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_ICURVEFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_ICURVEFACTORY_H_INCLUDED
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
    FWD_DECLARE_SMART_PTRS( ICurve );
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals );

    typedef std::tr1::function <ICurvePtr
                              (const LTQuant::GenericDataPtr,
                              const LeastSquaresResidualsPtr) > ICurveCreationFunction;

    /// A factory that builds curves
    class ICurveFactory : public DevCore::NameFactory <ICurveCreationFunction, 
                                                       ICurveFactory,
                                                       ICurveCreationFunction>
    {
    public:
        /// Creates a curve of the specified type, using
        /// the table passed as parameter
        static ICurvePtr createInstance(const std::string& iCurveTypeName,
                                        const LTQuant::GenericDataPtr interpolationDetailsTable,
                                        const LeastSquaresResidualsPtr leastSquaresResiduals);

        static ICurvePtr createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable,
                                        const LeastSquaresResidualsPtr leastSquaresResiduals);

        static ICurvePtr createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable);

        /// Creates a flat curve at the specified level
        static ICurvePtr createFlatCurve(const double y = 0.0);

        static ICurveFactory * const instance();

    private:
        explicit ICurveFactory();
        
        template <class TiCurve>
        static void registerICurve()
        {
            ICurveFactory::instance()->registerObject(TiCurve::getName(), TiCurve::createInstance);
        }
    };

}
#endif //__LIBRARY_PRICERS_FLEXYCF_ICURVEFACTORY_H_INCLUDED