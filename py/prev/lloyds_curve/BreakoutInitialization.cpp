/*****************************************************************************
    
	BreakoutInitialization


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "BreakoutInitialization.h"
#include "MultiCurveModel.h"
#include "KnotPoint.h"
#include "KnotPointFunctor.h"
#include "InstrumentCollector.h"


#include "Maths/LeastSquaresProblem.h"
#include "Maths/GaussNewtonSolver.h"
#include "Maths/LeastSquaresSolverFactory.h"
#include "SolverVariable.h"
#include "Data/GenericData.h"

//	IDeA
#include "DataExtraction.h"

using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF	
{
	BreakoutInitialization::BreakoutInitialization(const LTQuant::LeastSquaresSolverPtr& leastSquaresSolver,
												   const InitialSpotRates& initialSpotRates,
												   const double defaultSpotRate,
												   const CurveOrder& curveOrder,
												   const LeastSquaresRepresentationType::Enum_t lsrType):
		m_leastSquaresSolver(leastSquaresSolver),
		m_initialSpotRates(initialSpotRates),
		m_defaultSpotRate(defaultSpotRate),
		m_curveOrder(curveOrder),
		m_lsrType(lsrType)
	{
	}

	std::string BreakoutInitialization::getName()
	{
		return "Breakout";
	}

	BaseInitializationPtr BreakoutInitialization::create(const LTQuant::GenericDataPtr& initializationTable)
	{
		// get tables of initial spot rates and least squares solver (Gauss-Newton by default) parameters
		LTQuant::GenericDataPtr initialSpotRatesTable, solverParamsTable, curveOrderTable;
		
		// name and default name of the least squares solver
		const std::string defaultSolverTypeName(LTQuant::GaussNewtonSolver::getName());
		std::string solverTypeName(defaultSolverTypeName);

		if(initializationTable)
		{
			initializationTable->permissive_get<LTQuant::GenericDataPtr>("Spot Rates", 0, initialSpotRatesTable, LTQuant::GenericDataPtr());
			initializationTable->permissive_get<std::string>("Solver", 0, solverTypeName, defaultSolverTypeName);
			initializationTable->permissive_get<LTQuant::GenericDataPtr>("Solver Params", 0, solverParamsTable, LTQuant::GenericDataPtr());
			initializationTable->permissive_get<LTQuant::GenericDataPtr>("Order", 0, curveOrderTable, LTQuant::GenericDataPtr()); 
		}

		const LeastSquaresRepresentationType::Enum_t lsrType(permissiveGetLSRepresentationTypeFromSolverParamsTable(initializationTable));

		// Table of initial spot rates to be put into a map / dictionary
		// ensuring all curves are initialized
		std::string curveType;
		const std::string emptyString("");
		double spotRate, defaultSpotRate(0.03);
		InitialSpotRates initialSpotRates;

		if(initialSpotRatesTable)
		{
			const size_t nbSpotRates(IDeA::numberOfRecords(*initialSpotRatesTable));
			for(size_t cnt(0); cnt < nbSpotRates; ++cnt)
			{
				initialSpotRatesTable->permissive_get<std::string>("Type", cnt, curveType, emptyString);
				if(!(curveType.empty()))
				{
					initialSpotRatesTable->permissive_get<double>("Rate", cnt, spotRate, defaultSpotRate);
					
					// Default specifies the rate to use as an initial flat spot rate
					// for curves whose type is not specified in the initial spot rates table
					if(curveType == "Default")
					{
						defaultSpotRate = spotRate;
					}
					else
					{
						initialSpotRates[CurveType::getFromDescription(curveType)] = spotRate;
					}
				}
			}
		}
		
		// get the order in which to solve the curves:
		CurveOrder curveOrder;
		if(curveOrderTable)
		{
			const size_t nbCurveOrders(IDeA::numberOfRecords(*curveOrderTable));
			for(size_t cnt(0); cnt < nbCurveOrders; ++cnt)
			{
				curveOrderTable->permissive_get<std::string>("Order", cnt, curveType, emptyString);
				if(!(curveType.empty()))
				{
					curveOrder.push_back(CurveType::getFromDescription(curveType));
				}
			}
		}
		else
		{
			// open questions:
			// should we solve all tenors at once or individually by default?
			// should we solve tenor or discount first?
			// it is possible to specify to solve all tenor spine curve variables at once with:
			//	curveOrder.push_back(CurveType::AllTenors());
			
			curveOrder.push_back(CurveType::ON());
			curveOrder.push_back(CurveType::_1M());
			curveOrder.push_back(CurveType::_3M());
			curveOrder.push_back(CurveType::_6M());
			curveOrder.push_back(CurveType::_1Y());

			curveOrder.push_back(CurveType::Discount());
		}

		// Build the solver:
		LTQuant::LeastSquaresSolverPtr solver(LTQuant::LeastSquaresSolverFactory::createSolver(solverTypeName,
																							   solverParamsTable));
		
		return BaseInitializationPtr(new BreakoutInitialization(solver,
															    initialSpotRates,
															    defaultSpotRate, 
															    curveOrder,
																lsrType));
	}

	void BreakoutInitialization::doInitialize(BaseModel* const baseModel) const
	{
		// break-up initialization only makes sense for multi-curve models
		MultiCurveModel* const model(dynamic_cast<MultiCurveModel*>(baseModel));
		
		if(model)
		{
			// 1. Constant initialization per curve: isn't it an initialization in itself?
			// Only initialize kont-points if the initial spot rates object map is not empty
			// Note: spot rates for all curves - inner representation as spread should be handled internally (here or at MultiTenorModel level)
			if(m_initialSpotRates.empty())
			{
				LT_LOG << "*NOT* initializing knot-points with spot rates." << std::endl;
			}
			else
			{
				LT_LOG << "Initializing knot-points with spot rates." << std::endl;

				// 1.1 Base rate curve initialization:
				double baseInitialSpotRate(m_defaultSpotRate);
				InitialSpotRates::const_iterator iter(m_initialSpotRates.find(model->getBaseRate()));
				if(iter != m_initialSpotRates.end())
				{
					baseInitialSpotRate = iter->second;
				}
				model->initializeKnotPoints(model->getBaseRate(), baseInitialSpotRate);
				
				// 1.2 Tenor spine curves initialization:
				// First, ensure all tenor curves will be initialized by initializing everything at default spot rate
				model->initializeKnotPoints(CurveType::AllTenors(), m_defaultSpotRate - baseInitialSpotRate);

				// Then, see what's in the initial Spot Rates map
				for(InitialSpotRates::const_iterator iter(m_initialSpotRates.begin()); iter != m_initialSpotRates.end(); ++iter)
				{
					if(iter->first != model->getBaseRate() && iter->first != CurveType::Discount())
					{
						model->initializeKnotPoints(iter->first, iter->second - baseInitialSpotRate);
					}
				}
			
				// 1.3 Discount spine curve initialization
				if(model->getBaseRate() != CurveType::Discount())
				{
					double initialSpotRate(m_defaultSpotRate);
					iter = m_initialSpotRates.find(CurveType::Discount());
					if(iter != m_initialSpotRates.end())
					{
						initialSpotRate = iter->second;
					}
					model->initializeKnotPoints(CurveType::Discount(), initialSpotRate - baseInitialSpotRate);
				}
			}


			// 2. Break-out algorithm

			// InstrumentCollection will pick the instruments that were used in 
			// knot-point placement to place knot-points on the base curve
			// Knot-point functor to collect the instruments 'attached' to the knot-point
			// as their related unknown is added to the problem
			InstrumentCollector instrumentCollector(model);

			// If the base rate is not explicitly specified in the curve order in which
			//	to solve the curves, then start with it. In this case, this is done first  
			//	and independently on the specified order 
			// because the base rate curve should be built first
			// Solve the problem for the base curve related unknowns
			if(find(m_curveOrder.begin(), m_curveOrder.end(), model->getBaseRate()) == m_curveOrder.end())
			{
				LT_LOG << "Solving for base Rate " << model->getBaseRate() << endl;
				solveCurve(instrumentCollector, model, model->getBaseRate());
			}

			// Solve the other sub-problems:
			for(CurveOrder::const_iterator iter(m_curveOrder.begin()); iter != m_curveOrder.end(); ++iter)
			{
				LT_LOG << "Solving for " << (*iter) << endl;
				solveCurve(instrumentCollector, model, *iter);
			}

		}
	}

	void BreakoutInitialization::solveCurve(InstrumentCollector& instrumentCollector,
										    MultiCurveModel* const model,
										    const CurveTypeConstPtr& curveType) const
	{
		// 1. clear the instrument collector
		instrumentCollector.clear();

		// 2. create a least squares problem with the unknowns of the specified curve
		LTQuant::LeastSquaresProblemPtr leastSquaresProblem(instrumentCollector.createLeastSquaresProblem(model, curveType));
												 
		// 3. add the variables to this least squares problem, passing the instrument collector
		// to grab the instruments related to knot-point variables as they are added to the problem 
		model->addVariablesToProblem(leastSquaresProblem, curveType, instrumentCollector);
			
		// 3.5	Set the representation of least squares residuals for all problems
		instrumentCollector.setInstrumentResidualRepresentationType(m_lsrType);

		// 4. solve the least squares problem
		m_leastSquaresSolver->minimize(*leastSquaresProblem);
	}

}