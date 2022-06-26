/*****************************************************************************

	DV01
	
	Declaration of the function to calculate the DV01s of input instrument
	of a FlexYCF model
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __FlexYCF_DV01_H__
#define	__FlexYCF_DV01_H__


#include "InstrumentDelta.h"


namespace FlexYCF
{
	class BaseModel;

	void calculateDV01s(const BaseModel& model,
						InstrumentDeltaVector& dv01s);
						
}
#endif	//	__FlexYCF_DV01_H__
