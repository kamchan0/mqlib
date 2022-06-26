/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

#include "DictYieldCurve.h"
#include "DataExtraction.h"

#include "utils\Tenor.h"

#include "TolerantStraightLineTssInterpolation.h"
#include "ICurve.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
	void implyNonStandardTenor(double& tenor, double& w, double tolerance, const std::vector< std::pair<double,double> >& extraTenors)
	{
		for(size_t i=0; i<extraTenors.size(); ++i)
		{
			if(std::fabs(tenor - extraTenors[i].first) <=tolerance)
			{
				tenor = extraTenors[i].first;
				w     = extraTenors[i].second;
				return;
			}
		}
	}

	double TolerantStraightLineTssInterpolation::interpolate(const double tenor, const double flowTime) const
	{
		Curves::const_iterator lower(m_bucketedCurves.upperBound(tenor));
		Curves::const_iterator upper(m_bucketedCurves.lowerBound(tenor));

		// right side extrapolation  
		if(upper == m_bucketedCurves.end())
		{
			return (--upper)->second->evaluate(flowTime);
		}
		
		// left side extrapolation  
		if(upper == m_bucketedCurves.begin())
		{
			return upper->second->evaluate(flowTime);
		}

		// interpolation 
		--lower;
		double lowerSpread = lower->second->evaluate(flowTime);
		double upperSpread = upper->second->evaluate(flowTime);
	
		double tolerance = min(0.5*(upper->first - lower->first) * m_tolerance, m_maxTolerance);
		// flat
		if( fabs(tenor - lower->first) <= tolerance )
		{
			return lowerSpread;
		}
		if( fabs(upper->first - tenor) <= tolerance )
		{
			return upperSpread;
		}
		if(!m_extraTenorAndWeight.empty())
		{
			double impliedTenor = tenor;
			double w = 1.0;
			implyNonStandardTenor(impliedTenor, w, tolerance, m_extraTenorAndWeight);
			double weight = (impliedTenor - lower->first)/(upper->first - lower->first) * w;
			return weight * upperSpread + (1.0 - weight) * lowerSpread;
		}

		// linear
		double weight = (tenor - lower->first - tolerance)/(upper->first - lower->first - 2.0*tolerance);
		return weight * upperSpread + (1.0 - weight) * lowerSpread;
	}

	void TolerantStraightLineTssInterpolation::accumulateGradient(const double tenor, const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
		Curves::const_iterator lower(m_bucketedCurves.upperBound(tenor));
		Curves::const_iterator upper(m_bucketedCurves.lowerBound(tenor));
		
		// right side extrapolation  
		if(upper == m_bucketedCurves.end())
		{
			size_t ukpPos(0);
			for(Curves::const_iterator iter = m_bucketedCurves.begin(); iter != upper - 1; ++iter )
			{
				ukpPos += iter->second->getNumberOfUnknowns();
			}
			
			Curves::const_iterator last(upper - 1);
			last->second->accumulateGradient(flowTime, multiplier, gradientBegin + ukpPos, gradientBegin + ukpPos + last->second->getNumberOfUnknowns());
			return;
		}
			
		// left side extrapolation  
		if(upper == m_bucketedCurves.begin())
		{
			upper->second->accumulateGradient(flowTime, multiplier, gradientBegin, gradientBegin + upper->second->getNumberOfUnknowns());
			return;
		}
		
		// interpolation 
		--lower;
		double tolerance = min(0.5*(upper->first - lower->first) * m_tolerance, m_maxTolerance);
		
		size_t ukpPos(0);
		for(Curves::const_iterator iter = m_bucketedCurves.begin(); iter != lower; ++iter )
		{
			ukpPos += iter->second->getNumberOfUnknowns();
		}

		// flat
		if( fabs(tenor - lower->first) <= tolerance )
		{
			lower->second->accumulateGradient(flowTime, multiplier, gradientBegin + ukpPos, gradientBegin + ukpPos + lower->second->getNumberOfUnknowns());
			return;
		}
		if( fabs(upper->first - tenor) <= tolerance )
		{
			upper->second->accumulateGradient(flowTime, multiplier, gradientBegin + ukpPos + lower->second->getNumberOfUnknowns(), gradientBegin + ukpPos + lower->second->getNumberOfUnknowns()+ upper->second->getNumberOfUnknowns());
			return;
		}
		

        // linear interpolation 
		double weight = (tenor - lower->first - tolerance)/(upper->first - lower->first - 2.0*tolerance);
      
        const GradientIterator lowerStartPos(gradientBegin + ukpPos);
        const GradientIterator upperStartPos(lowerStartPos + lower->second->getNumberOfUnknowns());

        lower->second->accumulateGradient(flowTime, multiplier * (1.0 - weight), lowerStartPos, upperStartPos);
        upper->second->accumulateGradient(flowTime, multiplier * weight, upperStartPos, upperStartPos + upper->second->getNumberOfUnknowns());
    }

	void TolerantStraightLineTssInterpolation::accumulateGradient(const double tenor,
													  const double flowTime,
													  const double multiplier,
													  const GradientIterator gradientBegin,
													  const GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
        accumulateGradient(tenor, flowTime, multiplier, gradientBegin,gradientEnd);
	}
	
   
	void TolerantStraightLineTssInterpolation::finalize()
	{
	
		TenorSpreadSurface::TypedCurves::const_iterator iter(curves().begin());
		
		for( ; iter != curves().end(); ++iter)
		{
			m_bucketedCurves.insert(iter->first->getYearFraction(), iter->second);
		}
	}

    /**
        @brief Create a clone.

        The clone will directly access member data of its owning instance rather the original's owning instance.

        @param baseRate    The base rate to use.
        @param tenorCurves The owning instance's tenor curves.
    */
    BaseTssInterpolationPtr TolerantStraightLineTssInterpolation::clone(CurveTypeConstPtr const& baseRate, 
                                                            TenorSpreadSurface::TypedCurves& tenorCurves,
															const LTQuant::GenericDataPtr& tssInterpParams,
															CloneLookup& lookup) const
    {
        BaseTssInterpolationPtr clone(new TolerantStraightLineTssInterpolation(baseRate, tenorCurves, tssInterpParams));

        clone->finalize();
        return clone;
    }

	std::string TolerantStraightLineTssInterpolation::getName()
	{
		return "TolerantStraightLine";
	}

	BaseTssInterpolationPtr TolerantStraightLineTssInterpolation::createTssInterpolation(const CurveTypeConstPtr& baseRate,
																						 TenorSpreadSurface::TypedCurves& tenorCurves,
																						 const LTQuant::GenericDataPtr& tssInterpParams)
	{
		return BaseTssInterpolationPtr(new TolerantStraightLineTssInterpolation(baseRate, tenorCurves, tssInterpParams));
	}
	
	LTQuant::GenericDataPtr TolerantStraightLineTssInterpolation::TSSInterpParameters() const 
	{ 
		return  m_parameters; 
	}
	
	TolerantStraightLineTssInterpolation::TolerantStraightLineTssInterpolation(const CurveTypeConstPtr& baseRate,
													   TenorSpreadSurface::TypedCurves& tenorCurves,
													   const LTQuant::GenericDataPtr& tssInterpParams):
		BaseTssInterpolation(baseRate, tenorCurves)
	{
		m_parameters = tssInterpParams;

		// 0.2 is about 1 week for interpolation between 1m/3m
		IDeA::permissive_extract<double>(*tssInterpParams, IDeA_KEY(TSSIPARAMETERS, ROUNDING), m_tolerance, 0.2);
		if( m_tolerance < 0.0 || m_tolerance >= 1.0 )
		{
			ostringstream s;
			s << "TSSI rounding tolerance should be >= 0 and < 1, not " << m_tolerance;
			LT_THROW_ERROR(s.str());
		}
		std::string stringTenor;
		IDeA::permissive_extract<std::string>(*tssInterpParams, IDeA_KEY(TSSIPARAMETERS, MAXTOLERANCEINDAYS), stringTenor, "7C");
		LTQC::Tenor tenor(stringTenor.c_str());

		m_maxTolerance = tenor.asYearFraction();

		LTQuant::GenericDataPtr extraTenors;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(*tssInterpParams, IDeA_KEY(TSSIPARAMETERS, EXTRATENORS), extraTenors, LTQuant::GenericDataPtr());
		if(extraTenors)
		{
			for(size_t k=0; k < IDeA::numberOfRecords(*extraTenors); ++k)
			{
				std::string description = IDeA::extract<std::string>(*extraTenors, IDeA_KEY(EXTRATENORS, TENOR), k);
				if(!description.empty())
				{
					double weight = 1.0;
					IDeA::permissive_extract<double>(*extraTenors, IDeA_KEY(EXTRATENORS, WEIGHT),k,weight,1.0);
					CurveTypeConstPtr curve = CurveType::getFromDescription(description);
					double tenor = curve->getYearFraction();
					m_extraTenorAndWeight.push_back(std::make_pair(tenor,weight));
				}
			}
		}
	}
}