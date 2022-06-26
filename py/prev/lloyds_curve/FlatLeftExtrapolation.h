/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FLATLEFTEXTRAPOLATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FLATLEFTEXTRAPOLATION_H_INCLUDED
#pragma once

#include "LeftExtrapolationMethod.h"


namespace FlexYCF
{
    /// This class implements flat (i.e. constant) extrapolation
    /// on the left of the first knot-point.
    class FlatLeftExtrapolation : public LeftExtrapolationMethod
    {
    public:

        static std::string getName()
        {
            return "FlatLeft";
        }

        virtual double evaluate(const double x) const
        {
            return m_firstValue;
        }

        virtual void accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
        {
            if(!begin()->isKnown)
            {
                *gradientBegin += multiplier;
            }
        }

        virtual void onExtremalIteratorsSet()
        {
            if(begin() == end())
            {
                LT_THROW_ERROR("Not enough points for flat extrapolation.");
            }
            m_firstValue = begin()->y;
        }
       
        LeftExtrapolationMethodPtr clone() const
        {
            LeftExtrapolationMethodPtr retVal(new FlatLeftExtrapolation(*this));
            return retVal;
        }
    
    private:
        double m_firstValue;
    };

    DECLARE_SMART_PTRS( FlatLeftExtrapolation )
} 

#endif //__LIBRARY_PRICERS_FLEXYCF_FLATLEFTEXTRAPOLATION_H_INCLUDED