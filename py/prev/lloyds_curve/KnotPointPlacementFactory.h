/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_KNOTPOINTPLACEMENTFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_KNOTPOINTPLACEMENTFACTORY_H_INCLUDED
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
    FWD_DECLARE_SMART_PTRS( BaseKnotPointPlacement )

    typedef std::tr1::function<BaseKnotPointPlacementPtr (LTQuant::GenericDataPtr)> KnotPointPlacementCreationFunction;

    /// KnotPointPlacementFactory is a factory singleton to create concrete
    /// knot-point placement algorithms for models to be calibrated. 
    class KnotPointPlacementFactory : 
        public DevCore::NameFactory< KnotPointPlacementCreationFunction,
                                     KnotPointPlacementFactory,
                                     KnotPointPlacementCreationFunction >
    {
    public:
        static BaseKnotPointPlacementPtr createKnotPointPlacement(const std::string& objectID,
                                                                  const LTQuant::GenericDataPtr masterTable);
    private:
		friend class DevCore::NameFactory<	KnotPointPlacementCreationFunction,
											KnotPointPlacementFactory,
											KnotPointPlacementCreationFunction >;
		
		static KnotPointPlacementFactory* const instance();

        explicit KnotPointPlacementFactory();

        template<class KPP>
        static void registerKnotPointPlacement()
        {
            KnotPointPlacementFactory::instance()->registerObject(KPP::getName(), KPP::createInstance);
        }
    };  // KnotPointPlacementFactory

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_KNOTPOINTPLACEMENTFACTORY_H_INCLUDED