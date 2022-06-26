#include "stdafx.h"
#include "WeightedResidual.h"


namespace FlexYCF
{

    WeightedResidual::WeightedResidual(const double weight):
        m_weight(weight)
    {
        checkWeight();
    }
    
    double WeightedResidual::getValue() const
    {
        return m_weight * getValueImpl();
    }
  
    double WeightedResidual::getWeight() const
    {
        return m_weight;
    }

    void WeightedResidual::setWeight(const double weight)
    {
        m_weight = weight;
    }

    void WeightedResidual::checkWeight() const
    {
        if(m_weight < 0.0)
        {
            LT_THROW_ERROR( "Error with Residual weight: should be non-negative" );
        }
    }
}