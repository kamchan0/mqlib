#include "stdafx.h"
#include "ExtrapolationMethodFactory.h"
#include "ExtrapolationSpecs.h"
#include "FlatExtrapolationMethod.h"
#include "StraightLineExtrapolationMethod.h"

namespace FlexYCF
{
    template<>
    ExtrapolationMethodFactory<RightExtrapSpec>::ExtrapolationMethodFactory()
    {
        // Register only the extrapolation methods for which
        // a RightExtrapSpec template specialization of functions
        // evaluate and computeGradient has been provided
        registerExtrapolationMethod<FlatExtrapolationMethod>();
        registerExtrapolationMethod<StraightLineExtrapolationMethod>(); 
    }
}