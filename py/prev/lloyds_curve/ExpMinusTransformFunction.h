/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EXPMINUSTRANSFORMFUNCTION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXPMINUSTRANSFORMFUNCTION_H_INCLUDED
#pragma once

#include "TransformFunction.h"

namespace FlexYCF
{
    // Corresponds to the function f(x) = exp(-x)
    class ExpMinusTransformFunction : public TransformFunction
    {
    public:
        static std::string getName()
        {
            return "ExpMinus";
        }

        explicit ExpMinusTransformFunction() {}
        virtual ~ExpMinusTransformFunction() {}

        virtual double doTransform(const double x) const
        {
            return exp(-x);
        }
        
        virtual double doInverseTransform(const double x) const
        {
            return -log(x);
        }
        
        virtual double derivative(const double x) const
        {
            return -exp(-x);
        }


        /**
            @brief Clone this instance.

            I'm leaving the implementation in the header file as this file is only included in the factory.

            @return Return the clone.
        */
        virtual TransformFunctionPtr clone() const
        {
            return TransformFunctionPtr(new ExpMinusTransformFunction());
        }
    };
}
#endif //__LIBRARY_PRICERS_FLEXYCF_EXPMINUSTRANSFORMFUNCTION_H_INCLUDED