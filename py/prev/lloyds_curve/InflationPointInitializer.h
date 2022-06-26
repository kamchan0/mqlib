/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INFLATIONPOINTINITIALIZER_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INFLATIONPOINTINITIALIZER_H_INCLUDED

#include "KnotPointFunctor.h"
#include "InflationModel.h"

namespace FlexYCF
{
	// An initializer that initializes the knot-point variables of an InflationModel
	// from the quoted zero coupon rate of their related ILZCSwap
	class InflationPointInitializer: public IKnotPointFunctor
	{
	public:
		explicit InflationPointInitializer(InflationModel * const inflationModel):
			m_inflationModel(inflationModel, NullDeleter())
		{
		}
		
		virtual void operator() (KnotPoint & knotPoint) const;

	private:
		InflationModelPtr m_inflationModel;
	};

}

#endif //__LIBRARY_PRICERS_FLEXYCF_INFLATIONPOINTINITIALIZER_H_INCLUDED
