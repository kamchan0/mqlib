/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INFLATIONINITIALIZATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INFLATIONINITIALIZATION_H_INCLUDED


#include "BaseInitialization.h"

namespace FlexYCF
{
	class BaseModel;

	// A class to initialize the curve of an InflationModel
	// using zero-coupon inflation-linked swaps
	class InflationInitialization : public BaseInitialization
	{
	public:
		static std::string getName();
		static BaseInitializationPtr create(const LTQuant::GenericDataPtr& initializationTable);

	private:
		virtual void doInitialize(BaseModel* const model) const;
	};
}
#endif __LIBRARY_PRICERS_FLEXYCF_INFLATIONINITIALIZATION_H_INCLUDED