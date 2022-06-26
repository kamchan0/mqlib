#include "stdafx.h"
#include "FlatExtrapolationMethod.h"
#include "ExtrapolationSpecs.h"


namespace FlexYCF
{
    template<>
    void FlatExtrapolationMethod<LeftExtrapSpec>::initMemberVariable()
    {
        m_extremeKp = begin();
    }
}