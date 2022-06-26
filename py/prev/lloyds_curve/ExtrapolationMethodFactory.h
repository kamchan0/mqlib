/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODFACTORY_H_INCLUDED
#pragma once

#include "GenericExtrapolationMethod.h"

// DevCore
#include "DevCore/NameFactory.h"

namespace FlexYCF
{
    /// A generic factory to create generic extrapolation methods
    template <class ExtrapSpec>
    class ExtrapolationMethodFactory : 
        public DevCore::NameFactory< typename GenericExtrapolationMethod<ExtrapSpec>::CreationFunctionType,
                                     ExtrapolationMethodFactory<ExtrapSpec>,
                                     typename GenericExtrapolationMethod<ExtrapSpec>::CreationFunctionType >
    {
    public:
        typedef typename GenericExtrapolationMethod<ExtrapSpec>::Ptr GenericExtrapolationMethodPtr;

        static GenericExtrapolationMethodPtr createInstance(const std::string& extrapolationName,
                                                            const GradientFunction& interpolationGradientFunction)
        {
            return instance()->internalCreate(extrapolationName)(interpolationGradientFunction);
        }

		static ExtrapolationMethodFactory* const instance()
        {
            static ExtrapolationMethodFactory<ExtrapSpec> extrapMethodFactory;
            return &extrapMethodFactory;
        }

    private:

		// DGEM: Derived Generic Extrapolation Method
        template <template <typename> class DGEM>
        static void registerExtrapolationMethod()
        {
            ExtrapolationMethodFactory::instance()->registerObject(DGEM<ExtrapSpec>::getName(), DGEM<ExtrapSpec>::createInstance);
        }

        /// Registers are the constructors, but might depend on the specs
        /// so no generic implementation provided
        explicit ExtrapolationMethodFactory();
    };


}
#endif //__LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONMETHODFACTORY_H_INCLUDED