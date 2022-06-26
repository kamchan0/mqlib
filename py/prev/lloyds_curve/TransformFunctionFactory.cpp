/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

#include "stdafx.h"
#include "TransformFunctionFactory.h"
#include "NullTransformFunction.h"
#include "ExpMinusTransformFunction.h"

using namespace std;

namespace FlexYCF
{
    TransformFunctionFactory::TransformFunctionFactory()
    {
        registerObject("", TransformFunctionPtr(new NullTransformFunction));
        registerTransformFunction<NullTransformFunction>();
        registerTransformFunction<ExpMinusTransformFunction>();
    }

    TransformFunctionPtr TransformFunctionFactory::createInstance(const string& transformFunctionName)
    {
        return instance()->internalCreate(transformFunctionName);
    }

    TransformFunctionFactory* const TransformFunctionFactory::instance()
    {
        static TransformFunctionFactory theInstance;
        return &theInstance;
    }
}