/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "BasicTssInterpolation.h"
#include "CurveType.h"
#include "ICurve.h"
#include "FlexYCFCloneLookup.h"


using namespace LTQC;

namespace FlexYCF
{


	double BasicTssInterpolation::interpolate(const double tenor, const double flowTime) const
	{
		if(CurveType::getFromYearFraction(tenor) == baseRate())
        {
            return 0.0;
        }
        else
        {
			BucketedCurves::const_iterator lower(m_bucketedCurves.lowerBound(tenor));
			if(lower == m_bucketedCurves.end())
			{
				LT_THROW_ERROR("The curve tenor " <<  tenor << " is invalid.");
			}
			
			return lower->second->evaluate(flowTime);	
		}
	}
		
	void BasicTssInterpolation::accumulateGradient(const double tenor,
												   const double flowTime,
											 	   const double multiplier,
 												   const GradientIterator gradientBegin,
												   const GradientIterator gradientEnd) const
	{
		if(CurveType::getFromYearFraction(tenor) != baseRate())
        {
            BucketedCurves::const_iterator lower(m_bucketedCurves.lowerBound(tenor));

        	size_t ukpPos(0);
			TenorSpreadSurface::TypedCurves::const_iterator iter;
            
			for(iter = curves().begin(); iter != curves().end() && (iter->first->getYearFraction() < lower->first); ++iter)
			{
				ukpPos += iter->second->getNumberOfUnknowns();
			}
          
            if( (iter == curves().end()) || ( iter != curves().begin() && iter->first->getYearFraction() > lower->first))
            {
                --iter;
            }
            ukpPos -= iter->second->getNumberOfUnknowns();
			if(iter != curves().end()) 
			{
				iter->second->accumulateGradient(flowTime, multiplier, gradientBegin + ukpPos, gradientBegin + ukpPos + iter->second->getNumberOfUnknowns());
			}

		}
            

	}

	
	void BasicTssInterpolation::finalize()
	{
		TenorSpreadSurface::TypedCurves::const_iterator lastIter(curves().begin());
		
		for(TenorSpreadSurface::TypedCurves::const_iterator iter(lastIter + 1); iter != curves().end(); lastIter = iter++)
		{
			m_bucketedCurves.insert(0.5 * (lastIter->first->getYearFraction() + iter->first->getYearFraction()), lastIter->second);
		}

		m_bucketedCurves.insert(1000.0, lastIter->second);
	}
    /**
        @brief Create a clone.

        The clone will directly access member data of its owning instance rather the original's owning instance.

        @param baseRate    The base rate to use.
        @param tenorCurves The owning instance's tenor curves.
    */
    BaseTssInterpolationPtr BasicTssInterpolation::clone(CurveTypeConstPtr const& baseRate, 
                                                         TenorSpreadSurface::TypedCurves& tenorCurves,
														 const LTQuant::GenericDataPtr&   tssInterpParams,
														 CloneLookup& lookup) const
    {
		const BasicTssInterpolationPtr tssInterpolation(new BasicTssInterpolation(baseRate, tenorCurves, tssInterpParams));

        // Copy across the bucketed curves cloning as we go
        for (BucketedCurves::const_iterator it = m_bucketedCurves.begin(); it != m_bucketedCurves.end(); ++it)
        {
            // Keys are only doubles; curves are cloned
            tssInterpolation->m_bucketedCurves.insert((*it).first, lookup.get((*it).second));
        }
		
        return tssInterpolation;
    }

	std::string BasicTssInterpolation::getName()
	{
		return "Basic";
	}
	
	BaseTssInterpolationPtr BasicTssInterpolation::createTssInterpolation(const CurveTypeConstPtr& baseRate,
																					  TenorSpreadSurface::TypedCurves& tenorCurves,
																					  const LTQuant::GenericDataPtr& tssInterpParams)
	{
		return BaseTssInterpolationPtr(new BasicTssInterpolation(baseRate, tenorCurves, tssInterpParams));
	}
	
	BasicTssInterpolation::BasicTssInterpolation(const CurveTypeConstPtr& baseRate,
												 TenorSpreadSurface::TypedCurves& tenorCurves,
												 const LTQuant::GenericDataPtr&):
		BaseTssInterpolation(baseRate, tenorCurves)
	{
	}

}