/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESWITHANALYTICJACOBIAN_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESWITHANALYTICJACOBIAN_H_INCLUDED
#pragma once

#include "LeastSquaresProblemWithJacobian.h"

#include <functional>

namespace FlexYCF
{
    /// This class represents a least squares problem with a jacobian
    /// computed line by line with a gradient function.
    /// Note: this class is general, in the sense that it can be used for 
    /// any least squares problem that requires the jacobian to be computed.
    class LeastSquaresWithAnalyticJacobian : public LeastSquaresProblemWithJacobian
    {
    public:
        // The function to compute the gradient of the i-th function, corresponding
        // to how to fill the i-th line of the matrix (0 <= i < #functions)
        typedef std::tr1::function<void (const size_t, std::vector<double>&)> GradientFunc;

        LeastSquaresWithAnalyticJacobian(const size_t numberOfFunctions,
                                         const LTQuant::LeastSquaresProblem::EvalFunc& evalFunc,
                                         const LTQuant::LeastSquaresProblem::UpdateFunc& updateFunc,
                                         const GradientFunc& gradientFunc);

        virtual void jacobian(const double funcValues[], double jac[]) /*const*/ ; 
        virtual void jacobian(double jac[]) /*const*/;
    
    private:
        GradientFunc        m_gradientFunc;
        std::vector<double>      m_gradient;         // std::vector used to compute the gradients and fill the jacobian
    };  //  LeastSquaresWithAnalyticJacobian

    DECLARE_SMART_PTRS( LeastSquaresWithAnalyticJacobian )

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESWITHANALYTICJACOBIAN_H_INCLUDED