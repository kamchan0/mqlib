#ifndef __FLEXYCF_ITENORSURFACEINTERPOLATION_H__
#define __FLEXYCF_ITENORSURFACEINTERPOLATION_H__

#include "TenorSurface.h"

//	LTQuantCore
#include "Macros.h"

namespace FlexYCF
{
	class TenorSurfaceInterpolation
	{
	public:
		typedef TenorSurface::TypedCurves		TypedCurves;
		typedef TenorSurface::BucketedCurves    BucketedCurves;

        double interpolate(const double tenor, const double flowTime) const;
		void accumulateGradient(const double tenor, const double flowTime, const double multiplier, const GradientIterator gradientBegin, const GradientIterator gradientEnd) const;
		//void accumulateGradient(const double tenor,  const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, const CurveTypeConstPtr& curveType) const;
		void finalize();
        TenorSurfaceInterpolationPtr clone(TypedCurves& tenorCurves, CloneLookup& lookup) const;
		const TypedCurves& curves() const { return m_curves; } 
		TenorSurfaceInterpolation(TypedCurves& tenorCurves);
		static TenorSurfaceInterpolationPtr createTenorSurfaceInterpolation(TypedCurves& tenorCurves);
	private:
		TypedCurves&        m_curves;
		BucketedCurves      m_bucketedCurves;
	};
}
#endif