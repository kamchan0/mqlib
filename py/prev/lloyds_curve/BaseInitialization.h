/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BASEINITIALIZATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BASEINITIALIZATION_H_INCLUDED

#include "LTQuantInitial.h"

namespace FlexYCF
{
	class BaseModel;

	/// An interface to initialize a model
	class BaseInitialization
	{
	public:
		virtual ~BaseInitialization() = 0 { }

		void initialize(BaseModel* const model) const;

	private:
		virtual void doInitialize(BaseModel* const model) const = 0;  
	};

	DECLARE_SMART_PTRS( BaseInitialization )
}
#endif // __LIBRARY_PRICERS_FLEXYCF_BASEINITIALIZATION_H_INCLUDED