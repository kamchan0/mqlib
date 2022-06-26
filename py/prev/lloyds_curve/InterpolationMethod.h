/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONMETHOD_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONMETHOD_H_INCLUDED
#pragma once

#include "Clone.h"
#include "KnotPoint.h"
#include "KnotPoints.h"
#include "CurveAlgorithm.h"

#include <algorithm>

namespace FlexYCF
{
    // Interface for all interpolation methods
    class InterpolationMethod : public CurveAlgorithm, public LTQuant::IClone<InterpolationMethod>
    {
    public:
        
        virtual std::ostream& print(std::ostream& out) const
        {
            return out;
        }

		// only for MonotoneConvexSpline
		virtual void update()
		{
		}
    protected:
        explicit InterpolationMethod() { };
        virtual ~InterpolationMethod() = 0 { };

        // lowerBound and upperBound are used again and again by all concrete interpolation methods.
        //  They just encapsulate what doesn't change in the lower_bound and upper_bound calls.
        const_iterator lowerBound(const double x) const
        {
            return lower_bound(begin(), end(), x, KnotPoint::xCompare());       
        }

        const_iterator upperBound(const double x) const
        {
            return upper_bound(begin(), end(), x, KnotPoint::xCompare());       
        }

    };  //  InterpolationMethod

    DECLARE_SMART_PTRS( InterpolationMethod )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const InterpolationMethod& interpolationMethod)
		{
			return interpolationMethod.print(out);
		}
	}
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_INTERPOLATIONMETHOD_H_INCLUDED