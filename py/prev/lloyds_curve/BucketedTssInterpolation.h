/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __FLEXYCF_BUCKETEDTSSINTERPOLATION_H__
#define __FLEXYCF_BUCKETEDTSSINTERPOLATION_H__

#include "BaseTssInterpolation.h"
#include "TenorSpreadSurface.h"


namespace LTQuant
{
	class GDTblWrapper;
    typedef GDTblWrapper GenericData;
}

namespace FlexYCF
{
	class BucketedTssInterpolation: public BaseTssInterpolation
	{
	public:
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
		BucketedTssInterpolation(const CurveTypeConstPtr& baseRate,
								 TenorSpreadSurface::TypedCurves& tenorCurves,
								 const LTQuant::GenericDataPtr& tssInterpParams);

		typedef Dictionary<double, ICurvePtr>	BucketedCurves;
		BucketedCurves m_bucketedCurves;
	};
}
#endif	//	__FLEXYCF_BUCKETEDTSSINTERPOLATION_H__