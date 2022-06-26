/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __FLEXYCF_BASICTSSINTERPOLATION_H__
#define __FLEXYCF_BASICTSSINTERPOLATION_H__

#include "BaseTssInterpolation.h"


namespace FlexYCF
{


	// Represents flat interpolation between the tenors on the spread surface 
	//	(not all tenors in general)
	//	TODO: define exactly what is basic TSS interp.
	class BasicTssInterpolation: public BaseTssInterpolation
	{
	public:
		// Look for the LTQC::Tenor on the surface which is the closest to the specified Tenor
		//	and interpolate on this Tenor curve
		virtual double interpolate(const double tenor, const double flowTime) const;
		
		virtual void accumulateGradient(const double tenor,
										const double flowTime,
										const double multiplier,
										const GradientIterator gradientBegin,
										const GradientIterator gradientEnd) const;

		virtual void finalize();

        virtual BaseTssInterpolationPtr clone(CurveTypeConstPtr const& baseRate, 
                                              TenorSpreadSurface::TypedCurves& tenorCurves,
											  const LTQuant::GenericDataPtr&   tssInterpParams,
											  CloneLookup& lookup) const;

		static std::string getName();
		static BaseTssInterpolationPtr createTssInterpolation(const CurveTypeConstPtr& baseRate,
															  TenorSpreadSurface::TypedCurves& tenorCurves,
															  const LTQuant::GenericDataPtr& tssInterpParams);
        
	private:
		BasicTssInterpolation(const CurveTypeConstPtr& baseRate,
							  TenorSpreadSurface::TypedCurves& tenorCurves,
							  const LTQuant::GenericDataPtr& tssInterpParams);
		typedef Dictionary<double, ICurvePtr>	BucketedCurves;
		BucketedCurves m_bucketedCurves;
	};

	LTQC_DECLARE_SMART_PTRS( BasicTssInterpolation )
}
#endif	//	__FLEXYCF_BASICTSSINTERPOLATION_H__