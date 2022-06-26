/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESWITHJACOBIANPROXY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESWITHJACOBIANPROXY_H_INCLUDED
#pragma once

#include "LeastSquaresProblemWithJacobian.h"


namespace FlexYCF
{
    /// This class represents a least squares problem with the partial
    /// derivatives in the jacobian computed using finite differences.
    /// Note: this class is general, in the sense that it can be used for 
    /// any least squares problem that requires the jacobian to be computed.
    class LeastSquaresWithJacobianProxy : public LeastSquaresProblemWithJacobian
    {
    public:
        LeastSquaresWithJacobianProxy(const size_t numberOfFunctions,
                                      const LTQuant::LeastSquaresProblem::EvalFunc& evalFunc,
                                      const LTQuant::LeastSquaresProblem::UpdateFunc& onUpdate,
                                      const double epsilon);

        // These functions compute a proxy of the jacobian
        // using finite differences of first order.
        // We could compute provide 2nd and higher order if 
        // necessary
        void jacobian(double jac[]) /*const*/;
        void jacobian(const double funcValues[], double jac[]) /*const*/;

    private:
        const double    m_epsilon;              //   the shift used to compute the jacobian
        std::vector<double>  m_funcValues;           //   vectors used to compute the proxy of the jacobian
        std::vector<double>  m_shiftedFuncValues;    //   here to avoid unnecessary creation
    };  //  LeastSquaresWithJacobian

    DECLARE_SMART_PTRS( LeastSquaresWithJacobianProxy )

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESWITHJACOBIANPROXY_H_INCLUDED