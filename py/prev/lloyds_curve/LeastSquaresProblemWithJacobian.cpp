#include "stdafx.h"
#include "LeastSquaresProblemWithJacobian.h"


using namespace LTQC;

namespace FlexYCF
{
    #pragma warning(disable : 4355)

    LeastSquaresProblemWithJacobian::LeastSquaresProblemWithJacobian(const size_t numberOfFunctions,
                                                                     const LTQuant::LeastSquaresProblem::EvalFunc& evalFunc,
                                                                     const LTQuant::LeastSquaresProblem::UpdateFunc& onUpdate
                                                                    ):
        m_coreUpdate(onUpdate),
		LeastSquaresProblem(numberOfFunctions, evalFunc, [this] () {incrementAndUpdate();}),
        m_numberOfFunctions(getNumFunctions()),
        m_numberOfFunctionEvaluations(0),
        m_numberOfJacobianCalculations(0)
    {
    }

    void  LeastSquaresProblemWithJacobian::incrementAndUpdate()
    {
        ++m_numberOfFunctionEvaluations;
        if(m_coreUpdate)
        {
            m_coreUpdate();
        }
    }

    long LeastSquaresProblemWithJacobian::getNumberOfFunctionEvaluations() const
    {
        return m_numberOfFunctionEvaluations;
    }

    long LeastSquaresProblemWithJacobian::getNumberOfJacobianCalculations()  const
    {
        return m_numberOfJacobianCalculations;
    }
}   //  FlexYCF