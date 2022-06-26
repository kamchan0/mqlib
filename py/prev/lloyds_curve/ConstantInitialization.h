/*****************************************************************************

    ConstantInitialization

	Initializes knot-points from a constant spot rate
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CONSTANTINITIALIZATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CONSTANTINITIALIZATION_H_INCLUDED

#include "LTQuantInitial.h"

//	FlexYCF
#include "BaseInitialization.h"
#include "KnotPointFunctor.h"


namespace LTQuant
{
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
	class BaseModel;

	class ConstantInitialization : public BaseInitialization
	{
	public:
		explicit ConstantInitialization(const double initialSpotRate);

		static std::string getName();
		static BaseInitializationPtr create(const LTQuant::GenericDataPtr& initializationParamsTable);
	
	private:
		virtual void doInitialize(BaseModel* const model) const;
		double m_initialSpotRate;
	};

}
#endif //__LIBRARY_PRICERS_FLEXYCF_CONSTANTINITIALIZATION_H_INCLUDED