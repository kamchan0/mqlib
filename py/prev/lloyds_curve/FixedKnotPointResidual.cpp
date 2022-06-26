/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "FixedKnotPointResidual.h"

using namespace std;

namespace FlexYCF
{

    FixedKnotPointResidual::FixedKnotPointResidual(const double t,
                                                   const double y,
                                                   const EvalFunction& evalFunction,
                                                   const GradFunction& gradFunction,
                                                   const double weight):
        ExtraResidual(weight),
        m_t(t),
        m_y(y),
        m_evalFunction(evalFunction),
        m_gradFunction(gradFunction)
    {
    }

    double FixedKnotPointResidual::getValueImpl() const
    {
        return m_evalFunction(m_t) - m_y;
    }

    void  FixedKnotPointResidual::computeGradient(Gradient& gradient) const
    {
        m_gradFunction(m_t, gradient);
    }

    /**
        @brief Clone using lookup.

        Does not contain any contained instances so implementation is trivial.

        @return A clone of this instance.
    */
    ICloneLookupPtr FixedKnotPointResidual::cloneWithLookup(CloneLookup&) const
    {
        return ICloneLookupPtr(new FixedKnotPointResidual(*this));
    }
}   // FlexYCF