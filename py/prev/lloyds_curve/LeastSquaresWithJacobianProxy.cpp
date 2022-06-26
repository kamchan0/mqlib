#include "stdafx.h"
#include "LeastSquaresWithJacobianProxy.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    LeastSquaresWithJacobianProxy::LeastSquaresWithJacobianProxy(const size_t numberOfFunctions,
                                                                 const LTQuant::LeastSquaresProblem::EvalFunc& evalFunc,
                                                                 const LTQuant::LeastSquaresProblem::UpdateFunc& onUpdate,
                                                                 const double epsilon) :
        LeastSquaresProblemWithJacobian(numberOfFunctions, evalFunc, onUpdate),
        m_epsilon(epsilon),
        m_funcValues(numberOfFunctions),
        m_shiftedFuncValues(numberOfFunctions)
    {
    }

    void LeastSquaresWithJacobianProxy::jacobian(const double funcValues[], double jac[])
    {
        ++m_numberOfJacobianCalculations;

        for(size_t j(0); j < m_variables.size(); ++j)
        {
            operator[](j) += m_epsilon;   // shift j-th variable
            
            evaluate(&m_shiftedFuncValues[0]);

            operator[](j) -= m_epsilon;     // unshift j-th variable

            for(size_t i(0); i < m_numberOfFunctions; ++i)
            {
                jac[j * m_numberOfFunctions + i] = (m_shiftedFuncValues[i] - funcValues[i]) / m_epsilon;
            }
        }
    }

    void LeastSquaresWithJacobianProxy::jacobian(double jac[]) //const
    {
        evaluate(&m_funcValues[0]);
        jacobian(&m_funcValues[0], jac);
    }
}