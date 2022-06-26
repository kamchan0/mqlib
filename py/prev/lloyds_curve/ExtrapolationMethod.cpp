#include "stdafx.h"
#include "ExtrapolationMethod.h"


namespace FlexYCF
{

    void ExtrapolationMethod::accumulateInterpolationGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        m_interpolationGradientFunction(x, multiplier, gradientBegin, gradientEnd);
    }

}