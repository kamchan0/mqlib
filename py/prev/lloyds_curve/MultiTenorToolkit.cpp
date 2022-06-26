/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	IDeA
#include "DictYieldCurve.h"

//	FlexYCF
#include "GenericToolkit.h"
#include "MultiTenorModel.h"
#include "MultiTenorStripperKpp.h"
#include "FromInstrumentsInitialization.h"
#include "CompositeCurve.h"
#include "MonotoneConvexSplineInterpolation.h"

//	LTQuantLib
#include "Maths/LevenbergMarquardtSolver.h"
#include "Data/GenericData.h"


using namespace LTQC;

namespace FlexYCF
{
	
	LTQuant::GenericDataPtr GenericToolkit<MultiTenorModel>::getModelParameters() const
	{
		LTQuant::GenericDataPtr modelParametersData(IToolkit::getModelParameters());

		//	Base rate
		modelParametersData->set<std::string>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,BASERATE), 0, getBaseRate());

		return modelParametersData;

		/*
		LTQuant::GenericDataPtr modelParametersData(new LTQuant::GenericData(IDeA_TAG(FLEXYC_MODELPARAMETERS), 0));

		//	Base rate
		modelParametersData->set<std::string>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,BASERATE), 0, getBaseRate());

		// Curves interpolation
		modelParametersData->set<GenericDataPtr>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,CURVESINTERPOLATION), 0, getCurvesInterpolation());

		//	Knot-point placement
		modelParametersData->set<std::string>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,KPP), 0, getKppName());
		
		// Initialization
		modelParametersData->set<std::string>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,INITIALIZATION), 0, getInitializationName());
		modelParametersData->set<GenericDataPtr>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,INITIALIZATIONPARAMETERS), 0, getInitializationParameters());

		// Solver
		modelParametersData->set<std::string>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,SOLVER), 0, getSolverName());
		modelParametersData->set<GenericDataPtr>(IDeA_PARAM(FLEXYC_MODELPARAMETERS,SOLVERPARAMETERS), 0, getSolverParameters());

		return modelParametersData;
		*/
	}

	LTQuant::GenericDataPtr GenericToolkit<MultiTenorModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());

		std::string index("LIBOR");
		//	TODO: use a map for default index from currency
		if(currency() == "EUR")
		{
			index = "EURIB";
		}

		IDeA::inject(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), index);

		return curveParametersData;
	}

	std::string GenericToolkit<MultiTenorModel>::getKppName() const 
	{
		return MultiTenorStripperKpp::getName();
	}

	std::string GenericToolkit<MultiTenorModel>::getInitializationName() const 
	{
		return FromInstrumentsInitialization::getName();
	}

	std::string GenericToolkit<MultiTenorModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}

	template<>
	std::string GenericToolkit<MultiTenorModel>::getBaseRate() const 
	{
		//	should be currency-dependent
		return std::string("3M");
	}

	LTQuant::GenericDataPtr GenericToolkit<MultiTenorModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curvesInterpolationData(new LTQuant::GenericData("Curves Interpolation", 0));
	
		// By default, use the following spine curves: discount, ON, 1M, 3M, 6M, 1Y
		typedef std::vector<std::string> SpineCurves;
		SpineCurves spineCurves;
		spineCurves.push_back(CurveType::Discount()->getDescription());
		spineCurves.push_back(CurveType::ON()->getDescription());
		spineCurves.push_back(CurveType::_1M()->getDescription());
		spineCurves.push_back(CurveType::_3M()->getDescription());
		spineCurves.push_back(CurveType::_6M()->getDescription());
		spineCurves.push_back(CurveType::_1Y()->getDescription());

		const std::string baseRate(getBaseRate());

		for(SpineCurves::const_iterator iter(spineCurves.begin()); iter != spineCurves.end(); ++iter)
		{
			if(*iter == baseRate)
			{
				LTQuant::GenericDataPtr interpolatedCurveData(new LTQuant::GenericData(std::string(currency()).append(*iter), 0));
				LTQuant::GenericDataPtr interpolationCurveDetailsData(new LTQuant::GenericData("Interp Curve", 0));
				
				IDeA::inject(*interpolatedCurveData, IDeA_KEY(INTERPOLATIONDETAILS, CURVETYPE), FlexYCF::BaseCurve::getName());
				IDeA::inject(*interpolatedCurveData, IDeA_KEY(INTERPOLATIONDETAILS, INTERPOLATIONCURVEDETAILS), interpolationCurveDetailsData);
				IDeA::inject(*interpolatedCurveData, IDeA_KEY(INTERPOLATIONDETAILS, LEFTEXTRAP), FlatExtrapolationMethod<LeftExtrapSpec>::getName());
				IDeA::inject(*interpolatedCurveData, IDeA_KEY(INTERPOLATIONDETAILS, RIGHTEXTRAP), StraightLineExtrapolationMethod<RightExtrapSpec>::getName());

				//	Use a composite curve as an interpolation curve, with straight line on the left and monotone convex spline on the right
				//	Note: the "separation" is not provided here, it will be automatically set to the last future maturity once the knots are placed
				IDeA::inject(*interpolationCurveDetailsData, IDeA_KEY(INTERPOLATIONCURVEDETAILS, CURVETYPE), CompositeCurve::getName());
				IDeA::inject(*interpolationCurveDetailsData, IDeA_KEY(INTERPOLATIONCURVEDETAILS, LEFTCURVE), InterpolationCurveData<UkpCurve, StraightLineInterpolation>()());
					// StraightLineInterpolation::getName());
 				IDeA::inject(*interpolationCurveDetailsData, IDeA_KEY(INTERPOLATIONCURVEDETAILS, RIGHTCURVE), InterpolationCurveData<UkpCurve, MonotoneConvexSplineInterpolation>()());
					//	MonotoneConvexSplineInterpolation::getName());
 
				curvesInterpolationData->set<LTQuant::GenericDataPtr>(*iter, 0, interpolatedCurveData);
			}
			else
			{
				curvesInterpolationData->set<LTQuant::GenericDataPtr>(*iter, 0, InterpolatedCurveData<>()(currency(), *iter));
			}
		}
		
		return curvesInterpolationData;
	}


	LTQuant::GenericDataPtr GenericToolkit<MultiTenorModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));
	
		// no params for initialization from instruments

		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<MultiTenorModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("Analytical Jacobian", 0, 1);
		
		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);

		return solverParametersData;
	}
}