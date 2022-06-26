/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_GENERICEXTRAPOLATIONMETHOD_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_GENERICEXTRAPOLATIONMETHOD_H_INCLUDED
#pragma once
#include "ExtrapolationMethod.h"
#include "Gradient.h"
#include "Clone.h"

namespace FlexYCF
{
    /// A template class that defines the base class
    /// of each parallel hierarchy of extrapolation methods.
    template <class ExtrapSpec>
    class GenericExtrapolationMethod : public ExtrapolationMethod, 
                                       public LTQuant::IClone<GenericExtrapolationMethod<ExtrapSpec> >
    {
    public:
		typedef std::tr1::shared_ptr<GenericExtrapolationMethod<ExtrapSpec> > Ptr;
        typedef std::tr1::function<Ptr (const GradientFunction&)> CreationFunctionType;  
    };  // GenericExtrapolationMethod


    /// A bridge class to generically implements some functions common
    /// to all ExtrapolationMethods
    /// DGEM stands for Derived Generic Extrapolation Method
    template <class ExtrapSpec, 
              template <class> class DGEM>
    class GenericExtrapolationMethodWrap : public GenericExtrapolationMethod<ExtrapSpec>
    {
    public:
        static GenericExtrapolationMethod<ExtrapSpec>::Ptr
            createInstance(const GradientFunction& interpolationGradientFunction)
        {
            const GenericExtrapolationMethod<ExtrapSpec>::Ptr extrapolationMethod(new DGEM<ExtrapSpec>);

            extrapolationMethod->setInterpolationGradientFunction(interpolationGradientFunction);
            
            return extrapolationMethod;
        }
    };
              
}

#endif //__LIBRARY_PRICERS_FLEXYCF_GENERICEXTRAPOLATIONMETHOD_H_INCLUDED
