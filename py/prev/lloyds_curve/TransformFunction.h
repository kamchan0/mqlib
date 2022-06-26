/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TRANSFORMFUNCTION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TRANSFORMFUNCTION_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "Clone.h"

namespace FlexYCF
{
    // Represents a function, its inverse and its derivative
    class TransformFunction : public LTQuant::IClone<TransformFunction>
    {
    public:
        virtual ~TransformFunction() {}
        virtual double doTransform(const double x) const = 0;
        virtual double doInverseTransform(const double x) const = 0;
        virtual double derivative(const double x) const = 0;
    private:
    };

    DECLARE_SMART_PTRS(TransformFunction)
}
#endif //__LIBRARY_PRICERS_FLEXYCF_TRANSFORMFUNCTION_H_INCLUDED