/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TRANSFORMFUNCTIONFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TRANSFORMFUNCTIONFACTORY_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "TransformFunction.h"

// DevCore
#include "DevCore/NameFactory.h"

namespace FlexYCF
{

    /// TransformFunctionFactory
    class TransformFunctionFactory : public DevCore::NameFactory< TransformFunction, TransformFunctionFactory>
    {
    public:
        static TransformFunctionPtr createInstance(const std::string& transformFunctionName);

    private:
		friend class DevCore::NameFactory< TransformFunction, TransformFunctionFactory>;

        static TransformFunctionFactory* const instance();

        explicit TransformFunctionFactory();

		template<class TF>
        static void registerTransformFunction()
        {
            TransformFunctionFactory::instance()->registerObject(TF::getName(), TransformFunctionPtr(new TF));
        }
    };
}
#endif //__LIBRARY_PRICERS_FLEXYCF_TRANSFORMFUNCTIONFACTORY_H_INCLUDED