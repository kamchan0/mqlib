/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DISCRETEFORWARDSDIFFERENCE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DISCRETEFORWARDSDIFFERENCE_H_INCLUDED
#pragma once
#include "ExtraResidual.h"
#include "Gradient.h"


namespace FlexYCF
{
    /// Represents the difference (to minimize) between two consecutive 
    /// instantaneous flat forward rates. This should be used to smoothe
    ///  a curve.
    ///     
    /// NOTE: This class is not implemented for now.
    class DiscreteForwardsDifference : public ExtraResidual
    {
    public:
        explicit DiscreteForwardsDifference(const double weight = 1.0):
            ExtraResidual(weight)
        {
        }

        virtual void computeGradient(Gradient& gradient) const;

    protected:
        virtual double getValueImpl() const;

    private:
        // some private functions (now in StripperModel) to help with the calculations
        double discreteForward(const size_t /* index */) const
        {
            // TO DO
            return 0.0;
        }
    };

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_DISCRETEFORWARDSDIFFERENCE_H_INCLUDED
