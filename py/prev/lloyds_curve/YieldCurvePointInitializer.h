/*****************************************************************************

    YieldCurvePointInitializer

	A helper class to initialize knot-points from instrument rates.
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_YIELDCURVEPOINTINITIALIZER_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_YIELDCURVEPOINTINITIALIZER_H_INCLUDED

#include "KnotPointFunctor.h"

namespace FlexYCF
{
	class BaseModel;

	class YieldCurvePointInitializer : public IKnotPointFunctor
	{
	public:
		explicit YieldCurvePointInitializer(BaseModel* const model);

		virtual void operator()(KnotPoint& knotPoint) const;
	
	private:
		BaseModel* const	m_model;	
	};


}
#endif // __LIBRARY_PRICERS_FLEXYCF_YIELDCURVEPOINTINITIALIZER_H_INCLUDED