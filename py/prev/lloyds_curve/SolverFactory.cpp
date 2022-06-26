/*****************************************************************************
    
	Implementation of SolverFactory

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "BaseSolver.h"
#include "SolverFactory.h"
#include "DoNothingSolver.h"

//	LTQuantLib
#include "Maths/GaussNewtonSolver.h"
#include "Maths/LevenbergMarquardtSolver.h"
#include "Maths/CompositeLeastSquaresSolver.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{   
    SolverFactory::SolverFactory()
    {
		registerGenericSolver<LTQuant::GaussNewtonSolver>();
		registerGenericSolver<LTQuant::LevenbergMarquardtSolver>();
		registerGenericSolver<LTQuant::CompositeLeastSquaresSolver>();
		
		registerSolver<DoNothingSolver>();
	}

    BaseSolverPtr SolverFactory::createSolver(const string& solverName, 
                                              const LTQuant::GenericDataPtr& solverParametersTable)
    {
        return SolverFactory::create(solverName)(solverParametersTable);
    }

    SolverFactory* const SolverFactory::instance()
    {
        static SolverFactory solverFactory;
        return &solverFactory;
    }

}   //  FlexYCF