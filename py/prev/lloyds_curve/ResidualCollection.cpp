#include "stdafx.h"
#include "ResidualCollection.h"


namespace FlexYCF
{
    ResidualCollection::ResidualCollection(const double weight):
        m_weight(weight)
    {
    }

    double ResidualCollection::getResidual(const size_t index) const
    {
        checkIndex(index);
        return getResidualImpl(index);
    }
    
    double ResidualCollection::getWeight() const
    {
        return m_weight;
    }

    void ResidualCollection::setWeight(const double weight)
    {
         m_weight = weight;
    }

}   //  FlexYCF