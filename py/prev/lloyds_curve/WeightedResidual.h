/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

/// \file

#ifndef __LIBRARY_PRICERS_FLEXYCF_WEIGHTEDRESIDUAL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_WEIGHTEDRESIDUAL_H_INCLUDED
#pragma once
#include "Gradient.h"
#include "ICloneLookup.h"

namespace FlexYCF
{
    
    FWD_DECLARE_SMART_PTRS( BaseModel )
	FWD_DECLARE_SMART_PTRS( CurveType )

    /// An abstract class to represent a weighted residual term 
    /// in a weighted least squares problem. 
    class WeightedResidual : public ICloneLookup
    {
    public:
        explicit WeightedResidual(const double weight = 1.0);
        virtual ~WeightedResidual() = 0 { }

        /// Returns the value of the residual.
        double getValue() const;

        /// Computes the gradient (relative to the parameters to estimate) of the residual.
        virtual void computeGradient(Gradient& gradient) const = 0;

		/// Computes the gradient of the residuals, relative to the variables 
		///of the specified curve type
        virtual void computeGradient(Gradient& gradient, const CurveTypeConstPtr& curveType) const = 0;

		/// Returns the weight of the residual.
        double getWeight() const;

        /// Sets the weight of the residual.
        void setWeight(const double weight);

        
        virtual void update()
        {
            // Do nothing
        }

    private:
        /// 
        virtual double getValueImpl() const = 0;

        /// Checks that the weight is positive.
        void checkWeight() const;

        double m_weight;    
    };  //  WeightedResidual

    DECLARE_SMART_PTRS( WeightedResidual )

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_WEIGHTEDRESIDUAL_H_INCLUDED