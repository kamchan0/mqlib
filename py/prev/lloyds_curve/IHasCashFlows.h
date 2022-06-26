/*****************************************************************************

    IHasCashFlows

	Interface for classes that have cash flows.

    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __IDEA_FLEXYCF_IHASCASHFLOWS_H_INCLUDED
#define __IDEA_FLEXYCF_IHASCASHFLOWS_H_INCLUDED

#include "LTQuantInitial.h"


namespace LTQuant
{
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS;
}


namespace FlexYCF
{
	class BaseModel;

	//	An interface for objects that can return their cash flows
	class IHasCashFlows
	{
	public:
		virtual ~IHasCashFlows() = 0 { }

		virtual LTQuant::GenericDataPtr getCashFlows() = 0;

		virtual LTQuant::GenericDataPtr computeCashFlowPVs(const BaseModel& model) = 0;
	};

}
#endif	//	__IDEA_FLEXYCF_IHASCASHFLOWS_H_INCLUDED