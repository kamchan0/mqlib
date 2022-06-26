/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_NULLTRANSFORMFUNCTION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_NULLTRANSFORMFUNCTION_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"

namespace FlexYCF
{
    // Corresponds to the identity function f(x) = x
    class NullTransformFunction : public TransformFunction
    {
    public:
        static std::string getName()
        {
            return "Null";
        }

        explicit NullTransformFunction() {}
        virtual ~NullTransformFunction() {}
        virtual double doTransform(const double x) const
        {
            return x;
        }
        virtual double doInverseTransform(const double x) const
        {
            return x;
        }
        virtual double derivative(const double /* x */) const
        {
            return 1.0;
        }

        /**
            @brief Clone this instance.

            I'm leaving the implementation in the header file as this file is only included in the factory.

            @return Return the clone.
        */
        virtual TransformFunctionPtr clone() const
        {
            return TransformFunctionPtr(new NullTransformFunction());
        }
    };
}
#endif //__LIBRARY_PRICERS_FLEXYCF_NULLTRANSFORMFUNCTION_H_INCLUDED