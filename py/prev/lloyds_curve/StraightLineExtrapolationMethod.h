/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_STRAIGHTLINEEXTRAPOLATIONMETHOD_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_STRAIGHTLINEEXTRAPOLATIONMETHOD_H_INCLUDED
#pragma once

#include "GenericExtrapolationMethod.h"


namespace FlexYCF
{
    /// A generic class for straight line extrapolation methods.
    template<class ExtrapSpec>
    class StraightLineExtrapolationMethod : public GenericExtrapolationMethodWrap <ExtrapSpec, 
                                                                                   StraightLineExtrapolationMethod>
    {
    public:

        static std::string getName()
        {
            return "StraightLine";
        }

        /// ExtrapSpec-independent
        // extrapolation out of two knot-points (x[0],y[0]) and (x[1], y[1]) is:
        //  [(x - x[0]) * y[1] + (x[1] - x) * y[0]] / (x[1] - x[0])
        virtual double evaluate(const double x) const
        {
            if(x == m_kp0->x)
                return m_kp0->y;
            if(x == m_kp1->x)
                return m_kp1->y;
            return ((x - m_kp0->x) * m_kp1->y + (m_kp1->x - x) * m_kp0->y) * m_denInverse;
        }

        /// ExtrapSpec-independent
        virtual void accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
        { 
            accumulateInterpolationGradient(m_kp0->x, multiplier * (m_kp1->x - x) * m_denInverse, gradientBegin, gradientEnd);

            accumulateInterpolationGradient(m_kp1->x, multiplier * (x - m_kp0->x) * m_denInverse, gradientBegin, gradientEnd);
        }

        virtual void onExtremalIteratorsSet()
        {
            // needs at least two knot-points to do straight extrapolation
            if(begin() == end() || begin() + 1 == end())
            {
                LT_THROW_ERROR("Not enough points for straight line extrapolation.");
            }
            
            initMemberVariables();
        }
       
        GenericExtrapolationMethod<ExtrapSpec>::Ptr clone() const
        {
            const GenericExtrapolationMethod<ExtrapSpec>::Ptr retVal(new StraightLineExtrapolationMethod<ExtrapSpec>(*this));
            return retVal;
        }

    private:
        /// ExtrapSpec-dependent
        void initMemberVariables();

        double m_denInverse;
        const_iterator m_kp0, m_kp1;
    };
   
}

#endif //__LIBRARY_PRICERS_FLEXYCF_STRAIGHTLINEEXTRAPOLATIONMETHOD_H_INCLUDED