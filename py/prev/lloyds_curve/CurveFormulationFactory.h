/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CURVEFORMULATIONFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CURVEFORMULATIONFACTORY_H_INCLUDED
#pragma once

#include <functional>

// LTQuantLib
#include "LTQuantInitial.h"

// DevCore
#include "DevCore/NameFactory.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}
namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( CurveFormulation )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )

	typedef std::tr1::function< CurveFormulationPtr
                              (const LTQuant::GenericDataPtr&,
                              const std::string&,
							  const LeastSquaresResidualsPtr&)> CurveFormulationCreationFunction;

    /// CurveFormulationFactory is a factory to build instances of concrete curve formulations
    /// deriving from CurveFormulation.
    class CurveFormulationFactory : public DevCore::NameFactory<CurveFormulationCreationFunction, 
                                                                CurveFormulationFactory, 
                                                                CurveFormulationCreationFunction>
    {
    public:
    
        // create a CurveFormulation
        static CurveFormulationPtr createInstance(const std::string& curveFormulationName, 
                                                  const LTQuant::GenericDataPtr& InterpolationDetailsTable,
                                                  const std::string& curveDescription,
                                                  const LeastSquaresResidualsPtr& leastSquaresResiduals);
        /* Might be needed for more flexibility
        static CurveFormulationPtr createInstance(const std::string& curveFormulationName,
                                                  const std::string& curveDescription,
                                                  const LTQuant::GenericDataPtr masterTable);
        */
      

    private:
		friend class DevCore::NameFactory<CurveFormulationCreationFunction, 
										  CurveFormulationFactory, 
										  CurveFormulationCreationFunction>;
		
		static CurveFormulationFactory* const instance();

        explicit CurveFormulationFactory();

        template<class CF>
        static void registerCurveFormulation()
        {
            CurveFormulationFactory::instance()->registerObject(CF::getName(), CF::createInstance);
        }
    };  // CurveFormulationFactory
}

#endif //__LIBRARY_PRICERS_FLEXYCF_CURVEFORMULATIONFACTORY_H_INCLUDED