/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

#include "DictYieldCurve.h"

#include "GenericToolkit.h"
#include "StripperModel.h"
#include "FundingStripperModel.h"
#include "CTDModel.h"
#include "IndexStripperModel.h"
#include "FromInstrumentsInitialization.h"
#include "ConstantInitialization.h"
#include "StripperNoCashEndDateKpp.h"
#include "IndexFundingStripperKpp.h"
#include "SingleCurveDefaultKpp.h"
#include "MinusLogDiscountCurve.h"
#include "Maths/LevenbergMarquardtSolver.h"
#include "DoNothingSolver.h"
#include "Data/GenericData.h"
#include "StraightLineInterpolation.h"
#include "FlatExtrapolationMethod.h"
#include "StraightLineExtrapolationMethod.h"
#include "ExtrapolationSpecs.h"

using namespace LTQC;

namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<StripperModel>::getModelParameters() const
	{
		return IToolkit::getModelParameters();
		/*
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
		
		return modelParametersData;
		*/
	}	

	LTQuant::GenericDataPtr GenericToolkit<StripperModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());
		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), "LIBOR"); 
		
		return curveParametersData;
	}

	std::string GenericToolkit<StripperModel>::getKppName() const 
	{
        if(m_instrumentListData->doesTagExist("FxForward") || m_instrumentListData->doesTagExist("Forwards") || m_instrumentListData->doesTagExist("Commodity Futures") )
		{
			return SingleCurveDefaultKpp::getName();
		}
		return StripperNoCashEndDateKpp::getName();
	}

	std::string GenericToolkit<StripperModel>::getInitializationName() const 
	{
		return FromInstrumentsInitialization::getName();
	}

	std::string GenericToolkit<StripperModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}
	std::string GenericToolkit<StripperModel>::getBaseRate() const 
	{
		return std::string();
	}

	LTQuant::GenericDataPtr GenericToolkit<StripperModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData	(
			InterpolatedCurveData< UkpCurve, 
								   StraightLineInterpolation,
								   FlatExtrapolationMethod<LeftExtrapSpec>,
								   StraightLineExtrapolationMethod<RightExtrapSpec> >()(currency(), "") 
														);
		curveInterpolationData->set<std::string>("Formulation", 0, MinusLogDiscountCurve::getName());
	
		return curveInterpolationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<StripperModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));

		// no params for initialization from instruments

		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<StripperModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);
   
		return solverParametersData;
	}
}

namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<FundingStripperModel>::getModelParameters() const
	{
		return IToolkit::getModelParameters();
	}	

	LTQuant::GenericDataPtr GenericToolkit<FundingStripperModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());
		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), "LIBOR"); 
		
		return curveParametersData;
	}

	std::string GenericToolkit<FundingStripperModel>::getKppName() const 
	{
		return IndexFundingStripperKpp::getName();
	}

	std::string GenericToolkit<FundingStripperModel>::getInitializationName() const 
	{
		return FromInstrumentsInitialization::getName();
	}

	std::string GenericToolkit<FundingStripperModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}
	std::string GenericToolkit<FundingStripperModel>::getBaseRate() const 
	{
		return std::string();
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingStripperModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData	(InterpolatedCurveData< UkpCurve, StraightLineInterpolation, FlatExtrapolationMethod<LeftExtrapSpec>,StraightLineExtrapolationMethod<RightExtrapSpec> >()(currency(), "") );
		curveInterpolationData->set<std::string>("Formulation", 0, MinusLogDiscountCurve::getName());
	
		return curveInterpolationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingStripperModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));
		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingStripperModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);
   
		return solverParametersData;
	}
}

namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadStripperModel>::getModelParameters() const
	{
		return IToolkit::getModelParameters();
	}	

	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadStripperModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());
		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), "LIBOR"); 
		
		return curveParametersData;
	}

	std::string GenericToolkit<FundingSpreadStripperModel>::getKppName() const 
	{
		return IndexFundingStripperKpp::getName();
	}

	std::string GenericToolkit<FundingSpreadStripperModel>::getInitializationName() const 
	{
		return FromInstrumentsInitialization::getName();
	}

	std::string GenericToolkit<FundingSpreadStripperModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}
	std::string GenericToolkit<FundingSpreadStripperModel>::getBaseRate() const 
	{
		return std::string();
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadStripperModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData	(InterpolatedCurveData< UkpCurve, StraightLineInterpolation, FlatExtrapolationMethod<LeftExtrapSpec>,StraightLineExtrapolationMethod<RightExtrapSpec> >()(currency(), "") );
		curveInterpolationData->set<std::string>("Formulation", 0, MinusLogDiscountCurve::getName());
	
		return curveInterpolationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadStripperModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));
		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadStripperModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);
   
		return solverParametersData;
	}
}

namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadModel>::getModelParameters() const
	{
		return IToolkit::getModelParameters();
	}	

	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());
		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), "LIBOR"); 
		
		return curveParametersData;
	}

	std::string GenericToolkit<FundingSpreadModel>::getKppName() const 
	{
		return IndexFundingStripperKpp::getName();
	}

	std::string GenericToolkit<FundingSpreadModel>::getInitializationName() const 
	{
		return FromInstrumentsInitialization::getName();
	}

	std::string GenericToolkit<FundingSpreadModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}
	std::string GenericToolkit<FundingSpreadModel>::getBaseRate() const 
	{
		return std::string();
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData	(InterpolatedCurveData< UkpCurve, StraightLineInterpolation, FlatExtrapolationMethod<LeftExtrapSpec>,StraightLineExtrapolationMethod<RightExtrapSpec> >()(currency(), "") );
		curveInterpolationData->set<std::string>("Formulation", 0, MinusLogDiscountCurve::getName());
	
		return curveInterpolationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));
		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingSpreadModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);
   
		return solverParametersData;
	}
}


namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<IndexSpreadStripperModel>::getModelParameters() const
	{
		return IToolkit::getModelParameters();
	}	

	LTQuant::GenericDataPtr GenericToolkit<IndexSpreadStripperModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());
		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), "LIBOR"); 
		
		return curveParametersData;
	}

	std::string GenericToolkit<IndexSpreadStripperModel>::getKppName() const 
	{
		return IndexFundingStripperKpp::getName();
	}

	std::string GenericToolkit<IndexSpreadStripperModel>::getInitializationName() const 
	{
		return FromInstrumentsInitialization::getName();
	}

	std::string GenericToolkit<IndexSpreadStripperModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}
	std::string GenericToolkit<IndexSpreadStripperModel>::getBaseRate() const 
	{
		return std::string();
	}

	LTQuant::GenericDataPtr GenericToolkit<IndexSpreadStripperModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData	(InterpolatedCurveData< UkpCurve, StraightLineInterpolation, FlatExtrapolationMethod<LeftExtrapSpec>,StraightLineExtrapolationMethod<RightExtrapSpec> >()(currency(), "") );
		curveInterpolationData->set<std::string>("Formulation", 0, MinusLogDiscountCurve::getName());
	
		return curveInterpolationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<IndexSpreadStripperModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));
		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<IndexSpreadStripperModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);
   
		return solverParametersData;
	}
}

namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<FundingIndexSpreadStripperModel>::getModelParameters() const
	{
		return IToolkit::getModelParameters();
	}	

	LTQuant::GenericDataPtr GenericToolkit<FundingIndexSpreadStripperModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());
		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), "LIBOR"); 
		
		return curveParametersData;
	}

	std::string GenericToolkit<FundingIndexSpreadStripperModel>::getKppName() const 
	{
		return IndexFundingStripperKpp::getName();
	}

	std::string GenericToolkit<FundingIndexSpreadStripperModel>::getInitializationName() const 
	{
		return FromInstrumentsInitialization::getName();
	}

	std::string GenericToolkit<FundingIndexSpreadStripperModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}
	std::string GenericToolkit<FundingIndexSpreadStripperModel>::getBaseRate() const 
	{
		return std::string();
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingIndexSpreadStripperModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData	(InterpolatedCurveData< UkpCurve, StraightLineInterpolation, FlatExtrapolationMethod<LeftExtrapSpec>,StraightLineExtrapolationMethod<RightExtrapSpec> >()(currency(), "") );
		curveInterpolationData->set<std::string>("Formulation", 0, MinusLogDiscountCurve::getName());
	
		return curveInterpolationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingIndexSpreadStripperModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));
		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<FundingIndexSpreadStripperModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);
   
		return solverParametersData;
	}
}

namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<IndexBaseSpreadStripperModel>::getModelParameters() const
	{
		return IToolkit::getModelParameters();
	}	

	LTQuant::GenericDataPtr GenericToolkit<IndexBaseSpreadStripperModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());
		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), "LIBOR"); 
		
		return curveParametersData;
	}

	std::string GenericToolkit<IndexBaseSpreadStripperModel>::getKppName() const 
	{
		return IndexFundingStripperKpp::getName();
	}

	std::string GenericToolkit<IndexBaseSpreadStripperModel>::getInitializationName() const 
	{
		return FromInstrumentsInitialization::getName();
	}

	std::string GenericToolkit<IndexBaseSpreadStripperModel>::getSolverName() const 
	{
		return LTQuant::LevenbergMarquardtSolver::getName();
	}
	std::string GenericToolkit<IndexBaseSpreadStripperModel>::getBaseRate() const 
	{
		return std::string();
	}

	LTQuant::GenericDataPtr GenericToolkit<IndexBaseSpreadStripperModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData	(InterpolatedCurveData< UkpCurve, StraightLineInterpolation, FlatExtrapolationMethod<LeftExtrapSpec>,StraightLineExtrapolationMethod<RightExtrapSpec> >()(currency(), "") );
		curveInterpolationData->set<std::string>("Formulation", 0, MinusLogDiscountCurve::getName());
	
		return curveInterpolationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<IndexBaseSpreadStripperModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));
		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<IndexBaseSpreadStripperModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);
   
		return solverParametersData;
	}
}


namespace FlexYCF
{
	LTQuant::GenericDataPtr GenericToolkit<CTDModel>::getModelParameters() const
	{
		return IToolkit::getModelParameters();
	}	

	LTQuant::GenericDataPtr GenericToolkit<CTDModel>::getCurveParameters() const
	{
		LTQuant::GenericDataPtr curveParametersData(new LTQuant::GenericData("Curve Parameters", 0));

		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency());
		IDeA::inject<std::string>(*curveParametersData, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX), "LIBOR"); 
		
		return curveParametersData;
	}

	std::string GenericToolkit<CTDModel>::getKppName() const 
	{
		return IndexFundingStripperKpp::getName();
	}

	std::string GenericToolkit<CTDModel>::getInitializationName() const 
	{
		return FlexYCF::ConstantInitialization::getName();
	}

	std::string GenericToolkit<CTDModel>::getSolverName() const 
	{
		return FlexYCF::DoNothingSolver::getName();
	}
	std::string GenericToolkit<CTDModel>::getBaseRate() const 
	{
		return std::string();
	}

	LTQuant::GenericDataPtr GenericToolkit<CTDModel>::getCurvesInterpolation() const 
	{
		LTQuant::GenericDataPtr curveInterpolationData	(InterpolatedCurveData< UkpCurve, StraightLineInterpolation, FlatExtrapolationMethod<LeftExtrapSpec>,StraightLineExtrapolationMethod<RightExtrapSpec> >()(currency(), "") );
		curveInterpolationData->set<std::string>("Formulation", 0, MinusLogDiscountCurve::getName());
	
		return curveInterpolationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<CTDModel>::getInitializationParameters() const
	{
		LTQuant::GenericDataPtr initializationData(new LTQuant::GenericData("Initialization Params", 0));
		return initializationData;
	}

	LTQuant::GenericDataPtr GenericToolkit<CTDModel>::getSolverParameters() const
	{
		LTQuant::GenericDataPtr solverParametersData(new LTQuant::GenericData("Solver Params", 0));
	
		// Calculate jacobian matrices analytically (faster)
		solverParametersData->set<long>("AnalyticalJacobian", 0, 1);

		// Calculate the jacobian at each iteration
		solverParametersData->set<long>("1st Jacobian only", 0, 0);
   
		return solverParametersData;
	}
}