/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InstrumentComponent.h"

namespace FlexYCF
{
    GlobalComponentCache * InstrumentComponent::s_globalComponentCache = 0;
	bool InstrumentComponent::s_useCache = true;
}