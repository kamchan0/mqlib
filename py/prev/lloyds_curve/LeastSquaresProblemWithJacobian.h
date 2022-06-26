/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESPROBLEMWITHJACOBIAN_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESPROBLEMWITHJACOBIAN_H_INCLUDED
#pragma once

#include "Maths\LeastSquaresProblem.h"


namespace FlexYCF
{
    /// This abstract class is an interface for least squares problems that
    /// can provide a way to compute their jacobian.
    /// Note: for back-compatibility with the existing Levenberg-Marquardt functions
    /// the jacobian matrix, which has #functions lines and #variables columns, is
    /// represented column by column as a one-dimensional array of doubles. 
    class LeastSquaresProblemWithJacobian : public LTQuant::LeastSquaresProblem
    {
    public:
        explicit LeastSquaresProblemWithJacobian(const size_t numberOfFunctions,
                                                 const LTQuant::LeastSquaresProblem::EvalFunc& evalFunc,
                                                 const LTQuant::LeastSquaresProblem::UpdateFunc& onUpdate);


        // jac is an array that represents the jacobian matrix column by column
        virtual void jacobian(const double funcValues[], double jac[]) /*const*/ = 0; 
        virtual void jacobian(double jac[]) /*const*/ = 0;

        long getNumberOfFunctionEvaluations()   const;
        long getNumberOfJacobianCalculations()  const;

    private:
        // this function wraps the UpdateFunc passed in the constructor
        // count the number of function evaluations. It is always called
        // at the beginning of an evaluation, incrementing the # of evals
        // and calling the "real" update function.
        void incrementAndUpdate();

    protected:
        const size_t m_numberOfFunctions;
        long  m_numberOfFunctionEvaluations;
        long  m_numberOfJacobianCalculations;
        const LTQuant::LeastSquaresProblem::UpdateFunc m_coreUpdate;        
    };   //  LeastSquaresProblemWithJacobian

    DECLARE_SMART_PTRS( LeastSquaresProblemWithJacobian )

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESPROBLEMWITHJACOBIAN_H_INCLUDED