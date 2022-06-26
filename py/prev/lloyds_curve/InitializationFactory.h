/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INITIALIZATIONFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INITIALIZATIONFACTORY_H_INCLUDED
#pragma once

// LTQuantLib
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
    FWD_DECLARE_SMART_PTRS( BaseInitialization )

    typedef std::tr1::function< BaseInitializationPtr
                              (const LTQuant::GenericDataPtr&) > InitializationCreationFunction;

    /// A factory singleton to create instances of concrete initializations.
    class InitializationFactory : public DevCore::NameFactory<	InitializationCreationFunction, 
																InitializationFactory, 
																InitializationCreationFunction >
    {
    public:
        static BaseInitializationPtr createInitialization(const std::string& initializationName, 
														  const LTQuant::GenericDataPtr& initializationParametersTable);
		
    private:
        friend class DevCore::NameFactory<	InitializationCreationFunction, 
											InitializationFactory, 
											InitializationCreationFunction >;
		
		static InitializationFactory* const instance();
    
		explicit InitializationFactory();

        template<class T>
        static void registerInitialization()
        {
            InitializationFactory::instance()->registerObject(T::getName(), T::create);
        }
    };  // InitializationFactory

}   //   FlexYCF


#endif //__LIBRARY_PRICERS_FLEXYCF_INITIALIZATIONFACTORY_H_INCLUDED