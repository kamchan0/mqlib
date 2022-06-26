/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BSPLINEDEFININGFUNCTIONSFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BSPLINEDEFININGFUNCTIONSFACTORY_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"

// DevCore
#include "DevCore/NameFactory.h"

#include <functional>

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BSplineDefiningFunctions )

	typedef std::tr1::function < BSplineDefiningFunctionsPtr
                               (std::vector<double>&,
                               std::vector<double>&
                            ) > BSplineDefiningFunctionsCreationFunction;

    /// A factory class to build B-spline defining functions
    class BSplineDefiningFunctionsFactory : public DevCore::NameFactory < BSplineDefiningFunctionsCreationFunction, 
                                                                          BSplineDefiningFunctionsFactory,
                                                                          BSplineDefiningFunctionsCreationFunction   >
    {
    public:
        /// Creates B-Spline defining functions of a given type,
        /// with the knot sequence and the tension parameters specified
        static BSplineDefiningFunctionsPtr createBSplineDefiningFunctions(const std::string& splineTypeName,
                                                                          std::vector<double>& knotSequence, 
                                                                          std::vector<double>& tensionParameters);

    private:
        explicit BSplineDefiningFunctionsFactory();

		static BSplineDefiningFunctionsFactory* const instance();

        template<class BSDF>
        static void registerBSplineDefiningFunctions()
        {
            BSplineDefiningFunctionsFactory::instance()->registerObject(BSDF::getName(), BSDF::createInstance);
        }
    };  // BSplineDefiningFunctionsFactory

}
#endif //__LIBRARY_PRICERS_FLEXYCF_BSPLINEDEFININGFUNCTIONSFACTORY_H_INCLUDED