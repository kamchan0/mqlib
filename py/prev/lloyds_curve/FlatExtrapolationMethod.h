/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FLATEXTRAPOLATIONMETHOD_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FLATEXTRAPOLATIONMETHOD_H_INCLUDED
#pragma once

#include "GenericExtrapolationMethod.h"

namespace FlexYCF
{
    /// A generic class for flat extrapolation methods.
    template<class ExtrapSpec>
    class FlatExtrapolationMethod : public GenericExtrapolationMethodWrap <ExtrapSpec, 
                                                                           FlatExtrapolationMethod>
    {
    public:
        
        static std::string getName()
        {
            return "Flat";
        }

        virtual double evaluate(const double /* x */) const
        {
            return m_extremeKp->y;
        }

        virtual void accumulateGradient(const double /* x */, 
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd) const
        {
            accumulateInterpolationGradient(m_extremeKp->x, multiplier, gradientBegin, gradientEnd);
        }

        virtual void onExtremalIteratorsSet()
        {
            // needs at least one knot-point to do flat extrapolation
            if(begin() == end())
            {
                LT_THROW_ERROR("Not enough points for flat extrapolation.");
            }
            initMemberVariable();
        }

        
        GenericExtrapolationMethod<ExtrapSpec>::Ptr clone() const
        {
            const GenericExtrapolationMethod<ExtrapSpec>::Ptr retVal(new FlatExtrapolationMethod<ExtrapSpec>(*this));
            return retVal;
        }
        
    private:
        /// ExtrapSpec-dependent
        void initMemberVariable();

        const_iterator m_extremeKp;
    };
    
}

#endif //__LIBRARY_PRICERS_FLEXYCF_FLATEXTRAPOLATIONMETHOD_H_INCLUDED