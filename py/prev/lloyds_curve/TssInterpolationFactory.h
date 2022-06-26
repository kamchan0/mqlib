/*****************************************************************************

	TssInterpolationFactory

	The factory class to creation interpolation methods
	for the LTQC::Tenor spread surface.


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __FLEXYCF_TSSINTERPOLATIONFACTORY_H__
#define __FLEXYCF_TSSINTERPOLATIONFACTORY_H__

//	LTQuantLib
#include "LTQuantInitial.h"

//	FlexYCF
#include "TenorSpreadSurface.h"

//	LTQuantCore
#include "Macros.h"

// DevCore
#include "DevCore/NameFactory.h"

#include <functional>

namespace LTQuant
{
	class GDTblWrapper;
    typedef GDTblWrapper GenericData;
}


namespace FlexYCF
{
	LTQC_FWD_DECLARE_SMART_PTRS( BaseTssInterpolation );
	FWD_DECLARE_SMART_PTRS( CurveType );

	typedef std::tr1::function<BaseTssInterpolationPtr
							 (const CurveTypeConstPtr&,
							 TenorSpreadSurface::TypedCurves&,
							 const LTQuant::GenericDataPtr&)> TssInterpCreationFunction;
	
	//	The factory to create Tenor Spread Surface Interpolation methods
	class TssInterpolationFactory: public DevCore::NameFactory<TssInterpCreationFunction, 
															   TssInterpolationFactory,
															   TssInterpCreationFunction>
	{
	public:
		static BaseTssInterpolationPtr createTssInterpolation(const std::string& tssInterpolationName,
																		  const CurveTypeConstPtr& baseRate,
																		  TenorSpreadSurface::TypedCurves& tenorCurves,
																		  const LTQuant::GenericDataPtr& tssInterpParams);
	private:
		friend class DevCore::NameFactory<TssInterpCreationFunction, 
										  TssInterpolationFactory,
										  TssInterpCreationFunction>;

		static TssInterpolationFactory* instance();
		
		explicit TssInterpolationFactory();

		template<class TSSI>
		static void registerTssInterpolation()
		{
			TssInterpolationFactory::instance()->registerObject(TSSI::getName(), TSSI::createTssInterpolation);
		}
	};

}
#endif	//	__FLEXYCF_TSSINTERPOLATIONFACTORY_H__