/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "MultiCurveModel.h"
#include "CurveType.h"

using namespace LTQC;

namespace FlexYCF
{
    /**
        @brief Pseudo copy constructor using lookup.

        Use a lookup to preserved directed graph relationships.

        @param original The original instance being copied.
        @param lookup   A lookup of previously created clones.
    */
    MultiCurveModel::MultiCurveModel(MultiCurveModel const& original, CloneLookup& lookup) :
        BaseModel(original, lookup)
    {
    }

	// not sure a default implementation at this level makes sense
	double MultiCurveModel::getSpineDiscountFactor(const double flowTime,
												   const CurveTypeConstPtr& curveType) const
	{
		if(curveType == getBaseRate())
		{
			// true ignoring structure spread
			return getDiscountFactor(flowTime);
		}
		else if(curveType == CurveType::Discount())
		{
			// we use the fact that the base rate curve has no spread except structure
            if( getBaseRate() != CurveType::Discount() )
            {
			    return getDiscountFactor(flowTime) / getTenorDiscountFactor(flowTime, getBaseRate()->getYearFraction());
            }
            else
            {
                return getDiscountFactor(flowTime);
            }
		}
		else if(curveType->isTenor())
		{
            if( getBaseRate() != CurveType::Discount() )
            {
			    return getTenorDiscountFactor(flowTime, curveType->getYearFraction()) / getTenorDiscountFactor(flowTime, getBaseRate()->getYearFraction());
            }
            else
            {
                return getTenorDiscountFactor(flowTime, curveType->getYearFraction()) / getDiscountFactor(flowTime);
            }
		}
		else
		{
			LT_THROW_ERROR( "Invalid curve type specified to get the spine discount factor" );
		}
	}

}