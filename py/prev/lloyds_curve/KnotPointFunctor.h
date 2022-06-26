/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_KNOTPOINTFUNCTOR_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_KNOTPOINTFUNCTOR_H_INCLUDED

#include "KnotPoint.h"


namespace FlexYCF
{
	// A functor for operations on knot-point
	class IKnotPointFunctor
	{
	public:
		virtual void operator()(KnotPoint & ) const  = 0;
	};

	class doNothingKpFunctor : public IKnotPointFunctor
	{
	public:
		virtual void operator()(KnotPoint & ) const
		{ 
			// do nothing
		}
	};
}
#endif //__LIBRARY_PRICERS_FLEXYCF_KNOTPOINTFUNCTOR_H_INCLUDED