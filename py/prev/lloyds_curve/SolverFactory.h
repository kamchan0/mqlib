/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SOLVERFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SOLVERFACTORY_H_INCLUDED
#pragma once

#include "GenericSolver.h"

// DevCore
#include "DevCore/NameFactory.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BaseSolver )

	typedef std::tr1::function< BaseSolverPtr 
                              (const LTQuant::GenericDataPtr&) > SolverCreationFunction;

    /// A factory singleton to create instances of concrete solvers.
    class SolverFactory : public DevCore::NameFactory< SolverCreationFunction, 
                                                       SolverFactory, 
                                                       SolverCreationFunction >
    {
    public:
        static BaseSolverPtr createSolver(const std::string& solverName, 
                                          const LTQuant::GenericDataPtr& solverParametersTable);
		
    private:
		friend class DevCore::NameFactory< SolverCreationFunction, 
										   SolverFactory, 
										   SolverCreationFunction >;
		
		static SolverFactory* const instance();
        
		explicit SolverFactory();

        template<class S>
        static void registerSolver()
        {
            SolverFactory::instance()->registerObject(S::getName(), S::createInstance);
        }

		template<class LSS>
		static void registerGenericSolver()
		{
			SolverFactory::instance()->registerObject(LSS::getName(), GenericSolver<LSS>::createInstance);
		}
    };  // SolverFactory

}   //   FlexYCF


#endif //__LIBRARY_PRICERS_FLEXYCF_SOLVERFACTORY_H_INCLUDED