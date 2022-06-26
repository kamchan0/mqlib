/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FROMINSTRUMENTSINITIALIZATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FROMINSTRUMENTSINITIALIZATION_H_INCLUDED

#include "LTQuantInitial.h"
#include "BaseInitialization.h"


namespace FlexYCF
{
	// Initialize a model variables using initial guesses  	
	// from the rates/spreads/prices of the instruments
	class FromInstrumentsInitialization : public BaseInitialization
	{
	public:
		static std::string getName();
		static BaseInitializationPtr create(const LTQuant::GenericDataPtr& initializationTable);

	private:
		virtual void doInitialize(BaseModel* const model) const;
	};

	DECLARE_SMART_PTRS( FromInstrumentsInitialization )
}
#endif // __LIBRARY_PRICERS_FLEXYCF_YIELDCURVEINITIALIZATION_H_INCLUDED