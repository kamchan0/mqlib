/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CURVEINITIALIZATIONFUNCTION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CURVEINITIALIZATIONFUNCTION_H_INCLUDED

#include <functional>

namespace FlexYCF
{
	// The function used to initialize an ICurve
	// Note: we could use a function and modify the ICurve interface accordingly if necessary
	typedef std::tr1::function<double (const double)> CurveInitializationFunction;
}
#endif //__LIBRARY_PRICERS_FLEXYCF_CURVEINITIALIZATIONFUNCTION_H_INCLUDED