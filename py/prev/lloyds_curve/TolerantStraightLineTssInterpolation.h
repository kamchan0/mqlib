/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __FLEXYCF_TOLERANTSTRAIGHTLINETSSINTERPOLATION_H__
#define __FLEXYCF_TOLERANTSTRAIGHTLINETSSINTERPOLATION_H__


#include "BaseTssInterpolation.h"
#include "TenorSpreadSurface.h"


namespace FlexYCF
{
	class TolerantStraightLineTssInterpolation: public BaseTssInterpolation
	{
	public:
		// Interpolate on the LTQC::Tenor spread curve of the surface if the specified Tenor is
		//	within a few days of a surface LTQC::Tenor (this tolerance is Tenor-specific).
		//	Otherwise interpolate linearly between the two enclosing tenors
		virtual double interpolate(const double tenor, const double flowTime) const;
		
		virtual void accumulateGradient(const double tenor,
										const double flowTime,
										const double multiplier,
										const GradientIterator gradientBegin,
										const GradientIterator gradientEnd) const;

		virtual void accumulateGradient(const double tenor,
										const double flowTime,
										const double multiplier,
										const GradientIterator gradientBegin,
										const GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType) const;

        virtual void finalize();
		
		virtual BaseTssInterpolationPtr clone(CurveTypeConstPtr const& baseRate, 
                                              TenorSpreadSurface::TypedCurves& tenorCurves,
											  const LTQuant::GenericDataPtr&   tssInterpParams,
											  CloneLookup& lookup) const;
		
		virtual LTQuant::GenericDataPtr TSSInterpParameters() const;
		
		static std::string getName();

		static BaseTssInterpolationPtr createTssInterpolation(const CurveTypeConstPtr& baseRate,
															  TenorSpreadSurface::TypedCurves& tenorCurves,
															   const LTQuant::GenericDataPtr& tssInterpParams);

	private:
		TolerantStraightLineTssInterpolation(const CurveTypeConstPtr& baseRate,
											  TenorSpreadSurface::TypedCurves& tenorCurves,
											  const LTQuant::GenericDataPtr& tssInterpParams);
		
		typedef Dictionary<double, ICurvePtr>	Curves;
		Curves m_bucketedCurves;
		double m_tolerance;
		double m_maxTolerance;
		LTQuant::GenericDataPtr m_parameters;
		std::vector< std::pair<double,double> > m_extraTenorAndWeight;
	};

}
#endif	//	__FLEXYCF_TOLERANTSTRAIGHTLINETSSINTERPOLATION_H__

