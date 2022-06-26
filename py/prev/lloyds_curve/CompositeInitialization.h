/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_COMPOSITEINITIALIZATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_COMPOSITEINITIALIZATION_H_INCLUDED

#include "BaseInitialization.h"
#include "utils/Composite.h"


namespace FlexYCF
{
	// Represents a composition initialization 
	class CompositeInitialization : public LTQC::Composite< BaseInitialization,
															std::tr1::shared_ptr >
	{
	public:
		static std::string getName();
		static BaseInitializationPtr create(const LTQuant::GenericDataPtr& initializationTable);

	private:
		virtual void doInitialize(BaseModel* const model) const;

		void createAndAddChild(const std::string& childInitializationName,
							   const LTQuant::GenericDataPtr& childInitializationTable);
	};

	DECLARE_SMART_PTRS( CompositeInitialization )
}

#endif //__LIBRARY_PRICERS_FLEXYCF_COMPOSITEINITIALIZATION_H_INCLUDED
