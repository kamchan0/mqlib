/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ConstantInitialization.h"
#include "BaseModel.h"
#include "Data/GenericData.h"

using namespace LTQC;

namespace FlexYCF
{
	ConstantInitialization::ConstantInitialization(const double initialSpotRate):
		m_initialSpotRate(initialSpotRate)
	{
	}
		
	std::string ConstantInitialization::getName()
	{
		return "Constant";
	}

	BaseInitializationPtr ConstantInitialization::create(const LTQuant::GenericDataPtr& initializationParamsTable)
	{
		const double defaultSpotRate(0.03);
		double spotRate(defaultSpotRate);

		if(initializationParamsTable)
		{
			initializationParamsTable->permissive_get<double>("Spot Rate", 0, spotRate, defaultSpotRate);
		}
		
		return BaseInitializationPtr(new ConstantInitialization(spotRate));
	}
	
	void ConstantInitialization::doInitialize(BaseModel* const model) const
	{
		model->initializeKnotPoints();	// ??
	}
	
}