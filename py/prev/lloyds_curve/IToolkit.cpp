/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "IToolkit.h"
#include "DictYieldCurve.h"
#include "Data/GenericData.h"


using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
	
	LTQuant::GenericDataPtr defaultCacheData()
	{
		LTQuant::GenericDataPtr cacheData(new LTQuant::GenericData("Cache Parameters", 0));

		cacheData->set<long>("Discount Factor Cache", 0, 1);
		cacheData->set<long>("Tenor DF Cache", 0, 1);		
		cacheData->set<long>("Fwd Rate Cache", 0, 1);		
		cacheData->set<long>("Discounted Fwd Rate Cache", 0, 1);		
		cacheData->set<long>("Fxd Leg Cache", 0, 1);		
		cacheData->set<long>("Flt Leg Cache", 0, 1);

		return cacheData;
	}

	LTQuant::GenericDataPtr IToolkit::getModelParameters() const
	{
		LTQuant::GenericDataPtr modelParametersData(new LTQuant::GenericData(IDeA_TAG(FLEXYC_MODELPARAMETERS), 0));

		//	Curves Interpolation
		modelParametersData->set<GenericDataPtr>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,CURVESINTERPOLATION), 0, getCurvesInterpolation());

		//	Knot-point placement
		modelParametersData->set<std::string>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,KPP), 0, getKppName());

		//	Initialization
		modelParametersData->set<std::string>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,INITIALIZATION), 0, getInitializationName());
		modelParametersData->set<GenericDataPtr>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,INITIALIZATIONPARAMETERS), 0, getInitializationParameters());

		//	Solver
		modelParametersData->set<std::string>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,SOLVER), 0, getSolverName());
		modelParametersData->set<GenericDataPtr>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,SOLVERPARAMETERS), 0, getSolverParameters());
		
		//	Cache Parameters
		modelParametersData->set<GenericDataPtr>(IDeA_PARAM(FLEXYC_MODELPARAMETERS, CACHEPARAMETERS), 0, defaultCacheData());
		
		return modelParametersData;
	}

}