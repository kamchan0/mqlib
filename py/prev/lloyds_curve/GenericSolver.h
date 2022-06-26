/*****************************************************************************

	GenericSolver

	A generic wrapper of least squares solvers for FlexYCF calibration
	problems


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_GENERICSOLVER_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_GENERICSOLVER_H_INCLUDED
#pragma once

//	FlexYCF
#include "BaseSolver.h"
#include "BaseModel.h"
#include "LeastSquaresResiduals.h"
#include "ResidualsUtils.h"

#include "Maths/LeastSquaresProblem.h"
#include "Maths/LeastSquaresSolverFactory.h"
#include "utils/Macros.h"

#include "LTQuantException.h"

//	LTQuantCore
#include "Matrix.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
    FWD_DECLARE_SMART_PTRS( LeastSquaresSolver )
}


namespace FlexYCF
{
	// A template class that generically wrap a least squares solver
	// 
	template<class S>
    class GenericSolver : public BaseSolver
    {
    public:        
        static std::string getName();
        static BaseSolverPtr createInstance(const LTQuant::GenericDataPtr& solverParamsTable);

        /// Calibrates the model to the instruments
        virtual void solve(const CalibrationInstruments& calibrationInstruments, 
                           const BaseModelPtr baseModel);
    
    private:
        explicit GenericSolver(const LTQuant::GenericDataPtr& solverParamsTable);

		LTQuant::LeastSquaresSolverPtr			m_leastSquaresSolver;
		LeastSquaresRepresentationType::Enum_t	m_leastSquaresRepresentationType;
    };

	
	//	Implementation :
	template<class S>
	BaseSolverPtr GenericSolver<S>::createInstance(const LTQuant::GenericDataPtr& solverParamsTable)
	{
		return BaseSolverPtr(new GenericSolver<S>(solverParamsTable));
	}

	template<class S>
	GenericSolver<S>::GenericSolver(const LTQuant::GenericDataPtr& solverParamsTable)
	{
		m_leastSquaresSolver = LTQuant::LeastSquaresSolverFactory::createSolver<S>(solverParamsTable);
		m_leastSquaresRepresentationType = permissiveGetLSRepresentationTypeFromSolverParamsTable(solverParamsTable);
	}

	template<class S>
    void GenericSolver<S>::solve(const CalibrationInstruments& /* calibrationInstruments */, 
								 const BaseModelPtr baseModel)
	{
		// Note: finalize and initializeKnotPoints done in FlexYCFZeroCurve::rebuildCurveFromData()
        const LeastSquaresResidualsPtr leastSquaresResiduals(baseModel->getLeastSquaresResiduals());

		//	Set the least squares residuals representation (PV/Rate)
		leastSquaresResiduals->setInstrumentResidualRepresentationType(m_leastSquaresRepresentationType);

		const LTQuant::LeastSquaresProblemPtr leastSquaresProblem(leastSquaresResiduals->createLeastSquaresProblem());
		baseModel->addVariablesToProblem(leastSquaresProblem);

        try
        {
            LT_LOG << "#variables: " << leastSquaresProblem->getNumDimensions() << std::endl;
            LT_LOG << "#functions: " << leastSquaresProblem->getNumFunctions() << std::endl;
            LT_LOG << "Residuals before calibration : " << std::endl << (*leastSquaresResiduals) << std::endl;
        }
		catch(std::exception& exc)
        {
            LT_LOG << "ERROR: " << exc.what() << std::endl;
        }

		if(leastSquaresProblem->getNumFunctions() == 0 && leastSquaresProblem->getNumDimensions() == 0)
		{
			// No variables and no functions (e.g. only structure instruments), do nothing
		}
		else
		{
			const double residualsNorm(m_leastSquaresSolver->minimize(*leastSquaresProblem));
			
			/*
			if(residualsNorm > m_leastSquaresSolver->getResidualsNormThreshold())
			{
				LT_THROW_ERROR( "Residuals Norm (" << residualsNorm << ") is higher than the residuals norm threshold set for this solver (" 
					<< m_leastSquaresSolver->getResidualsNormThreshold() << ")");
			}
			*/
			// ** Note ** :
			//	once solved, each solver should store some information like:
			//	- the number of iterations (max and on solved)
			//	- the residuals norm

			//	Calculate the jacobian and set it to the model
			LTQC::Matrix jacobian(leastSquaresProblem->getNumFunctions(), leastSquaresProblem->getNumDimensions());
			leastSquaresProblem->computeJacobian(jacobian);
			baseModel->setJacobian(jacobian);

			/*
			//	Debug: output Jacobian:
			std::ofstream jacFile;
			jacFile.open("C:\\jac.txt");
			jacFile << jacobian;
			jacFile.close();
			*/

			// *!* # function evaluations is usually greater than # iterations when computing a proxy of the jacobian *!*
			LT_INFO //	<< "Optimisation successful with actual number of iteration being " << m_numIterations 
					//	<< " against the specified max of " << m_maxFunctionEvaluations << endl
				<< "Norm of residuals: " << residualsNorm << std::endl;
	        
			try
			{
				LT_LOG << "Residuals after calibration : " << endl << (*leastSquaresResiduals) << std::endl;
			} 
			catch(std::exception& exc)
			{
				LT_LOG << "ERROR: " << exc.what() << endl;
			}
		}
	}


}
#endif // __LIBRARY_PRICERS_FLEXYCF_GENERICSOLVER_H_INCLUDED