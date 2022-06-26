#include "stdafx.h"

//	FlexYCF
#include "TenorSurfaceInterpolation.h"
#include "CurveType.h"
#include "ICurve.h"
#include "FlexYCFCloneLookup.h"
#include "Interpolation.h"
#include "ICurveFactory.h"
#include "ICurve.h"
#include "CurveUtils.h"
#include "CurveFormulationFactory.h"
#include "CurveFormulation.h"
#include "MinusLogDiscountCurve.h"
#include "UkpCurve.h"
#include "Maths\Problem.h"
#include "TblConversion.h"

using namespace LTQC;

namespace FlexYCF
{
	TenorSurfaceInterpolation::TenorSurfaceInterpolation(TypedCurves& tenorCurves) : m_curves(tenorCurves) {}
	
	void TenorSurfaceInterpolation::finalize()
	{
		auto lastIter = m_curves.begin();
		
		for(auto iter = lastIter + 1; iter != m_curves.end(); lastIter = iter++)
		{
			m_bucketedCurves.insert(0.5 * (lastIter->first->getYearFraction() + iter->first->getYearFraction()), lastIter->second);
		}
		m_bucketedCurves.insert(1000.0, lastIter->second);
	}

	double TenorSurfaceInterpolation::interpolate(const double tenor, const double flowTime) const
	{
		auto lower(m_bucketedCurves.lowerBound(tenor));
		if(lower == m_bucketedCurves.end())
		{
			LT_THROW_ERROR("The curve tenor " <<  tenor << " is invalid.");
		}
		return (*lower).second->getDiscountFactor(flowTime);	
	}
		
	void TenorSurfaceInterpolation::accumulateGradient(const double tenor, const double flowTime, const double multiplier, const GradientIterator gradientBegin, const GradientIterator gradientEnd) const
	{
        auto lower(m_bucketedCurves.lowerBound(tenor));

        size_t ukpPos(0);
        auto  iter = m_bucketedCurves.begin();   
		for( ; iter != m_bucketedCurves.end() && (*iter).first < (*lower).first ; ++iter)
		{
			ukpPos += iter->second->getNumberOfUnknowns();
		}
          
		if(iter != m_bucketedCurves.end()) 
		{
			iter->second->accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin + ukpPos, gradientBegin + ukpPos + iter->second->getNumberOfUnknowns());
		}
	}

	/*void TenorSurfaceInterpolation::accumulateGradient(const double tenor,  const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, const CurveTypeConstPtr& curveType) const
	{
		if(curveType == CurveType::AllTenors())
		{
			accumulateGradient(tenor, flowTime, multiplier, gradientBegin, gradientEnd);
		}
		else
		{
			TypedCurves::const_iterator iter(curves().lowerBound(curveType));
			if(iter != curves().end() && iter->first == curveType)
			{
				iter->second->accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
			}
		}
	}*/

    TenorSurfaceInterpolationPtr TenorSurfaceInterpolation::clone(TypedCurves& tenorCurves, CloneLookup& lookup) const
    {
		const TenorSurfaceInterpolationPtr ts(new TenorSurfaceInterpolation(tenorCurves));
        for(auto it = m_curves.begin(); it != m_curves.end(); ++it)
        {
            ts->m_curves.insert((*it).first, lookup.get((*it).second));
        }
		ts->finalize();
        return ts;
    }

	TenorSurfaceInterpolationPtr TenorSurfaceInterpolation::createTenorSurfaceInterpolation(TenorSurface::TypedCurves& tenorCurves)
	{
		return TenorSurfaceInterpolationPtr(new TenorSurfaceInterpolation(tenorCurves));
	}
}