#include "stdafx.h"
#include "LeastSquaresWithAnalyticJacobian.h"
#include "BaseModel.h"


using namespace LTQC;

namespace FlexYCF
{
    
    LeastSquaresWithAnalyticJacobian::LeastSquaresWithAnalyticJacobian(const size_t numberOfFunctions,
                                                                       const LTQuant::LeastSquaresProblem::EvalFunc& evalFunc,
                                                                       const LTQuant::LeastSquaresProblem::UpdateFunc& updateFunc,
                                                                       const GradientFunc& gradientFunc):
        LeastSquaresProblemWithJacobian(numberOfFunctions, evalFunc, updateFunc),
        m_gradientFunc(gradientFunc),
        m_gradient(getNumDimensions())
    {
    }

    void LeastSquaresWithAnalyticJacobian::jacobian(const double funcValues[],
                                                    double jac[]) /*const*/ 
    {
        &funcValues; // IH stop C4100
        jacobian(jac);
    }

    void LeastSquaresWithAnalyticJacobian::jacobian(double jac[]) 
    {
        ++m_numberOfJacobianCalculations;
        for(size_t i(0); i < m_numberOfFunctions; ++i)
        {
            m_gradient.resize(m_variables.size());
            std::fill(m_gradient.begin(), m_gradient.end(), 0.0);

            m_gradientFunc(i, m_gradient);

            // Fill the jacobian (making sure it is "read" column by column, whereas gradients are computed row by row):
            for(size_t j(0); j < m_variables.size(); ++j)
            {
                jac[j * m_numberOfFunctions + i] = m_gradient[j];
            }        
        }
    }

}   //  FlexYCF