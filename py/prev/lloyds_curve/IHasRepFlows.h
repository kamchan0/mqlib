/*****************************************************************************

    IHasRepFlows

	Interface for classes that calculate their replicating flows.
    

    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __IHasRepFlows_H__
#define __IHasRepFlows_H__

#include "AssetDomain.h"

namespace IDeA
{
	template<class T>
	class RepFlowsData;
}

namespace FlexYCF
{
	class BaseModel;

	template<class T>
	class IHasRepFlows
	{
	public:
		virtual ~IHasRepFlows() = 0 { }

        virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<T>& repFlows) = 0;
	};
}

#endif	//	__IHasRepFlows_H__