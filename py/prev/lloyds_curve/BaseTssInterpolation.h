/*****************************************************************************

    BaseTssInterpolation
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __FLEXYCF_ITENORSPREADSURFACEINTERPOLATION_H__
#define __FLEXYCF_ITENORSPREADSURFACEINTERPOLATION_H__

//	FlexYCF
#include "TenorSpreadSurface.h"

//	LTQuantCore
#include "Macros.h"


namespace FlexYCF
{

	//	Base class that represents an interpolation between the index/tenor curves
	//	on the Tenor spread surface
	class BaseTssInterpolation
	{
	public:
        virtual ~BaseTssInterpolation()
        {
        }

        virtual double interpolate(const double tenor, const double flowTime) const = 0;
		
		//	Accumulates the gradient on a subset (most generally a curve) of the surface,
		//	as specified by the curve type
		//	Note: the specified curve type indicates whether the gradient iterators begin/end refer
		//	to all the curves or a specific curve. The latter is used for break-out initialization
		virtual void accumulateGradient(const double tenor,
										const double flowTime,
										const double multiplier,
										const GradientIterator gradientBegin,
										const GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType) const;
		
		//	Accumulates the gradient of the whole surface
		virtual void accumulateGradient(const double tenor,
										const double flowTime,
										const double multiplier,
										const GradientIterator gradientBegin,
										const GradientIterator gradientEnd) const = 0;

		virtual void finalize() {	}

        /**
            @brief Create a clone of this instance.

            Since this instance directly access (by reference) member data of its owning instance, then the clone must 
            have to reference to the member of its owning instance. So we're cloning the behaviour and not the actual 
            data.

            @param baseRate    The base rate to use.
            @param tenorCurves The new LTQC::Tenor curves to use.
        */
        virtual BaseTssInterpolationPtr clone(CurveTypeConstPtr const& baseRate,
                                              TenorSpreadSurface::TypedCurves& tenorCurves,
											  const LTQuant::GenericDataPtr&   tssInterpParams,
											  CloneLookup& lookup) const = 0;

		virtual LTQuant::GenericDataPtr TSSInterpParameters() const;

	protected:
		typedef TenorSpreadSurface::TypedCurves		TypedCurves;

		inline const CurveTypeConstPtr& baseRate() const
		{
			return m_baseRate;
		}

		inline const TenorSpreadSurface::TypedCurves& curves() const
		{
			return m_curves;
		}

		explicit BaseTssInterpolation(const CurveTypeConstPtr& baseRate,
									  TenorSpreadSurface::TypedCurves& tenorCurves):
			m_baseRate(baseRate),
			m_curves(tenorCurves)
		{
		}
		
			
	private:
		CurveTypeConstPtr					m_baseRate;
		TenorSpreadSurface::TypedCurves&	m_curves;
	};

	LTQC_DECLARE_SMART_PTRS( BaseTssInterpolation )
}
#endif	//	__FLEXYCF_ITENORSPREADSURFACEINTERPOLATION_H__