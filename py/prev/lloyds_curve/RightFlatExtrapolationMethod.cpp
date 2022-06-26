#include "stdafx.h"
#include "FlatExtrapolationMethod.h"
#include "ExtrapolationSpecs.h"


namespace FlexYCF
{
    template<>
    void FlatExtrapolationMethod<RightExtrapSpec>::initMemberVariable()
    {
        m_extremeKp = end() - 1;
    }
}