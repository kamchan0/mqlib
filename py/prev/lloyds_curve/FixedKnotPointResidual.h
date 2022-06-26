/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FIXEDKNOTPOINTRESIDUAL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FIXEDKNOTPOINTRESIDUAL_H_INCLUDED
#pragma once
#include "ExtraResidual.h"
#include "Gradient.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BaseModel )

    /// Represents a fixed-knot point constraint for curve whose unknowns
    /// are not knot-point values, e.g. tension splines.
    class FixedKnotPointResidual : public ExtraResidual     // WeightedResidual directly?
    {
    private:
        typedef std::tr1::function<double (const double)> EvalFunction;
        typedef std::tr1::function<void (const double, std::vector<double>&) > GradFunction;
    public:
        explicit FixedKnotPointResidual(const double t,
                                        const double y,
                                        const EvalFunction& evalFunction,
                                        const GradFunction& gradFunction,
                                        const double weight = 1.0);

        virtual void computeGradient(Gradient& gradient) const;
 
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup&) const;

    private:
        virtual double getValueImpl() const;
    
        const double m_t;
        const double m_y;
        const EvalFunction m_evalFunction;
        const GradFunction m_gradFunction;
    };
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FIXEDKNOTPOINTRESIDUAL_H_INCLUDED
