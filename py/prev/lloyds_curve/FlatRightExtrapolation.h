/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FLATRIGHTEXTRAPOLATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FLATRIGHTEXTRAPOLATION_H_INCLUDED
#pragma once

#include "RightExtrapolationMethod.h"


namespace FlexYCF
{

    class FlatRightExtrapolation : public RightExtrapolationMethod
    {
    public:
        static std::string getName()
        {
            return "FlatRight";
        }

        virtual double evaluate(const double x) const
        {
            return m_lastValue;
        }

        virtual void accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
        {
            const_iterator lastKnot(end() - 1);
            if(!lastKnot->isKnown)
            {
                *(gradientEnd - 1) += multiplier;
            }
        }

        virtual void onExtremalIteratorsSet()
        {
            if(begin() == end())
            {
                LT_THROW_ERROR("Not enough points for flat extrapolation.");
            }
            const const_iterator lastKnotPoint(end() - 1);
            m_lastValue = lastKnotPoint->y;
        }
         
        RightExtrapolationMethodPtr clone() const
        {
            RightExtrapolationMethodPtr retVal(new FlatRightExtrapolation(*this));
            return retVal;
        }

    private:
        double m_lastValue;

    }; //  FlatRightExtrapolation

    DECLARE_SMART_PTRS( FlatRightExtrapolation )
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FLATRIGHTEXTRAPOLATION_H_INCLUDED