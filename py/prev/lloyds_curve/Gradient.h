/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_GRADIENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_GRADIENT_H_INCLUDED

#include "LTQuantInitial.h"

#include <functional>

namespace FlexYCF
{
    typedef std::vector<double>  Gradient;
    typedef std::vector<double>::iterator GradientIterator;
    typedef std::tr1::function<void (const double, double, GradientIterator&, GradientIterator&)> GradientFunction;
    
    /// Multiplies the gradient by the scalar specified and
    /// replaces its contents by the result
    void multiply(const double scalar, Gradient& gradient); 

    /// Adds two first gradients and put the result in the third one
    void addTo(const Gradient& gradLeft, const Gradient& gradRight, Gradient& gradResult);
    
    /// Subtracts second gradient from the first and put the result in the third one
    void subtractTo(const Gradient& gradLeft, const Gradient& gradRight, Gradient& gradResult);

    /// Resize the specified gradient at the given size and fills it with zeros 
    void fillWithZeros(Gradient& vect, const size_t size);

}

#endif //__LIBRARY_PRICERS_FLEXYCF_GRADIENT_H_INCLUDED