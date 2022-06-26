/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONMETHODFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONMETHODFACTORY_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "InterpolationMethod.h"

// DevCore
#include "DevCore/NameFactory.h"

namespace FlexYCF
{
    // *!* Probably need to have a more sophisticated factory for interpolation methods.
    // *!* Indeed, even though the default constructor will do for most of the interpolations,
    // *!* for some of them we would like to add some extra-flexibility. For instance,
    // *!* we'd like to be able to specify the interpolant for the first and last interval
    // *!* of the bi-quadratic. This can be done by adding Left/Right extrapolation methods
    // *!* to the bi-quadratic interpolation, and having the factory return a static
    // *!* constructor function

    /// InterpolationMethodFactory
    class InterpolationMethodFactory : public DevCore::NameFactory< InterpolationMethod, 
																	InterpolationMethodFactory>
    {
    public:
        static InterpolationMethodPtr create(const std::string& interpolationMethodName);

    private:
		friend class  DevCore::NameFactory< InterpolationMethod, 
											InterpolationMethodFactory>;
		
		static InterpolationMethodFactory* instance();

		explicit InterpolationMethodFactory();
        
        template<class IM>
        static void registerInterpolation()
        {
            InterpolationMethodFactory::instance()->registerObject(IM::getName(), InterpolationMethodPtr(new IM));
        }
    };
}
#endif //__LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONMETHODFACTORY_H_INCLUDED