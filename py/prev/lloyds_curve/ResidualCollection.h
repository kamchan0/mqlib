/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_RESIDUALCOLLECTION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_RESIDUALCOLLECTION_H_INCLUDED
#pragma once

#include "Gradient.h"

namespace FlexYCF
{
	FWD_DECLARE_SMART_PTRS( CurveType )


	/// This abstract class represents a collection of residuals
    class ResidualCollection
    {
    public:
        explicit ResidualCollection(const double weight = 1.0);

        double getResidual(const size_t index) const;
        virtual void computeGradient(const size_t index, Gradient& gradient) const = 0;
		virtual void computeGradient(const size_t index, 
									 Gradient& gradient, 
									 const CurveTypeConstPtr& curveType) const = 0;
        
		double getWeight() const;
        void setWeight(const double weight);

        virtual void update()
        {
            // Do nothing by default
        }

		/// Clear the collection
		virtual void clear() = 0;

    protected:
        virtual double getResidualImpl(const size_t index) const = 0;
    
    private:
        virtual void checkIndex(const size_t index) const = 0;
        
        double m_weight;    // a weight that applies to the whole collection of the residuals, multiplying each weight of single residuals
    };

}
#endif //__LIBRARY_PRICERS_FLEXYCF_WEIGHTEDCOLLECTION_H_INCLUDED