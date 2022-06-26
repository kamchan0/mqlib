#include "stdafx.h"

//	IDeA
#include "DictYieldCurve.h"
//#include "IDeA/src/FlexYCF/LevenbergMarquardtSolver.h"

//	FlexYCF
#include "GenericToolkit.h"
#include "InflationModel.h"
#include "InflationKpp.h"
#include "InflationInitialization.h"
#include "StraightLineInterpolation.h"
#include "ExpMinusTransformFunction.h"

//	LTQuantLib
#include "Maths/LevenbergMarquardtSolver.h"


using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<InflationModel>::getModelParameters() const
	{
		const LTQuant::GenericDataPtr modelParametersData(new LTQuant::GenericData(IDeA_TAG(FLEXYC_MODELPARAMETERS), 0));

		IDeA::inject<std::string>(*modelParametersData, IDeA_KEY(FLEXYC_MODELPARAMETERS,SOLVER), getSolverName());
		IDeA::inject<GenericDataPtr>(*modelParametersData, IDeA_KEY(FLEXYC_MODELPARAMETERS,SOLVERPARAMETERS), getSolverParameters());
		IDeA::inject<std::string>(*modelParametersData, IDeA_KEY(FLEXYC_MODELPARAMETERS,KPP), getKppName());
		IDeA::inject<std::string>(*modelParametersData, IDeA_KEY(FLEXYC_MODELPARAMETERS,INITIALIZATION), getInitializationName());
		IDeA::inject<GenericDataPtr>(*modelParametersData, IDeA_KEY(FLEXYC_MODELPARAMETERS,INITIALIZATIONPARAMETERS), getInitializationParameters());

		//	IDeA::inject<GenericDataPtr>(*modelParametersData, DeA_KEY(FLEXYC_MODELPARAMETERS,CURVESINTERPOLATION), getCurvesInterpolation());
		//	IDeA::inject<std::string>(*modelParametersData, IDeA_KEY(FLEXYC_MODELPARAMETERS,BASERATE), getBaseRate()
		
		return modelParametersData;
	}

	LTQuant::GenericDataPtr GenericToolkit<InflationModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		// Those are inflation-specific
		curveParametersData->set<std::string>("Interpolator", 0, StraightLineInterpolation::getName());
		curveParametersData->set<std::string>("Transform", 0, ExpMinusTransformFunction::getName());
		curveParametersData->set<std::string>("Seasonal.Interpolator", 0, StraightLineInterpolation::getName());

		return curveParametersData;
	}

	// Not relevant!
	LTQuant::GenericDataPtr GenericToolkit<InflationModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData;

		return curveInterpolationData;
	}

	template<>
	std::string GenericToolkit<InflationModel>::getKppName() const 
	{
		return InflationKpp::getName();
	}

	template<>
	std::string GenericToolkit<InflationModel>::getInitializationName() const 
	{
		return InflationInitialization::getName();
	}

	template<>
	LTQuant::GenericDataPtr GenericToolkit<InflationModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));

		// no params for inflation initialization 

		return initializationData;
	}

	template<>
	std::string GenericToolkit<InflationModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}

	template<>
	LTQuant::GenericDataPtr GenericToolkit<InflationModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData(IDeA_PARAM(FLEXYC_MODELPARAMETERS, SOLVERPARAMETERS), 0));
	
		// Calculate jacobian matrices analytically (faster)
		//solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		//solverParametersData->set<long>("1st Jacobian only", 0, 0);

		return solverParametersData;
	}

	template<>
	std::string GenericToolkit<InflationModel>::getBaseRate() const 
	{
		return std::string();
	}
	/*
	// Inflation-specific tables
	void GenericToolkit<InflationModel>::setStaticData(const LTQuant::GenericDataPtr& inflationCurveData) const 
	{
		const std::string staticDataLabel("Static Data");
		LTQuant::GenericDataPtr staticData(new LTQuant::GenericData(staticDataLabel, 0));

		// TO DO

		inflationCurveData->set<LTQuant::GenericDataPtr>(staticDataLabel, 0, staticData);
	}

	void GenericToolkit<InflationModel>::setEventsData(const LTQuant::GenericDataPtr& inflationCurveData) const 
	{
		const std::string eventsDataLabel("Events");
		LTQuant::GenericDataPtr eventsData(new LTQuant::GenericData(eventsDataLabel, 0));
		
		// TO DO

		inflationCurveData->set<LTQuant::GenericDataPtr>(eventsDataLabel, 0, eventsData);
	}
	*/
	
}