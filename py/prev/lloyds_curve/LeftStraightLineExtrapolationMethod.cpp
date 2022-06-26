#include "stdafx.h"
#include "StraightLineExtrapolationMethod.h"
#include "ExtrapolationSpecs.h"

namespace FlexYCF
{
    template<>
    void StraightLineExtrapolationMethod<LeftExtrapSpec>::initMemberVariables()
    {
        m_kp0 = begin();
        m_kp1 = begin() + 1;

        // inverse of the denominator
        m_denInverse = m_kp1->x - m_kp0->x;
        
        if(m_denInverse <= 0.0)
        {
            LT_THROW_ERROR("The difference between the first two knots must be > 0.");
        }

        m_denInverse = 1.0 / m_denInverse;
    }
}