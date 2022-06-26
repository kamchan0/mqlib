/*****************************************************************************
    
	BaseInitialization

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "BaseInitialization.h"
#include "BaseModel.h"


using namespace LTQC;

namespace FlexYCF
{
	void BaseInitialization::initialize(BaseModel* const model) const
	{
		doInitialize(model);
		model->onInitialized();
	}
}