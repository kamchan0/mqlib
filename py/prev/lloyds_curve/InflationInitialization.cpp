/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InflationInitialization.h"
#include "InflationPointInitializer.h"
#include "Maths/Problem.h"
#include "Maths/LeastSquaresProblem.h"


using namespace LTQC;

namespace FlexYCF
{
	string InflationInitialization::getName()
	{
		return "Inflation";
	}

	// No table used for construction for now
	BaseInitializationPtr InflationInitialization::create(const LTQuant::GenericDataPtr&)
	{
		return BaseInitializationPtr(new InflationInitialization);
	}

	void InflationInitialization::doInitialize(BaseModel* const model) const
	{
		LT_LOG << "Inflation Initialization" << std::endl; 

		InflationModel* const inflationModel(dynamic_cast<InflationModel*>(model));
		InflationPointInitializer inflationPtInitializer(inflationModel);

		// just to conform to interface, solving each knot-point directly:
		// no knot-point should be added to the problem
		LTQuant::ProblemPtr uselessProblem(new LTQuant::LeastSquaresProblem);

		if(inflationModel)
		{
			inflationModel->addVariablesToProblem(uselessProblem, inflationPtInitializer);
		}	
	}
}