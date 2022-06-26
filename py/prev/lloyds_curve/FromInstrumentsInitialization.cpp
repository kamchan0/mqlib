/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "FromInstrumentsInitialization.h"
#include "YieldCurvePointInitializer.h"
#include "BaseModel.h"
#include "SingleCurveModel.h"
#include "Data/GenericData.h"
#include "Maths/Problem.h"
#include "Maths/LeastSquaresProblem.h"


using namespace LTQC;

namespace FlexYCF
{
	std::string FromInstrumentsInitialization::getName()
	{
		return "FromInstruments";
	}

	BaseInitializationPtr FromInstrumentsInitialization::create(const LTQuant::GenericDataPtr& /* initializationTable */)
	{
		return BaseInitializationPtr(new FromInstrumentsInitialization);
	}

	void FromInstrumentsInitialization::doInitialize(BaseModel* const model) const
	{
		LT_LOG << "Initialization from instruments" << std::endl;
		
		
		/*SingleCurveModel* const singleCurveModel(dynamic_cast<SingleCurveModel*>(model));

		if(singleCurveModel)
		{*/
			YieldCurvePointInitializer initializer(/*singleCurveM*/model);	
			LTQuant::ProblemPtr uselessProblem(new LTQuant::LeastSquaresProblem);
		
			model->addVariablesToProblem(uselessProblem, initializer);	
		/*
			singleCurveModel->addVariablesToProblem(uselessProblem, initializer);
		}
		*/
	}
}