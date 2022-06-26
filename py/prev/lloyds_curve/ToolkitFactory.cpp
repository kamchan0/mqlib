/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ToolkitFactory.h"
#include "StripperModel.h"
#include "FundingStripperModel.h"
#include "IndexStripperModel.h"
#include "MultiTenorModel.h"
#include "InflationModel.h"
#include "CTDModel.h"


using namespace LTQC;

namespace FlexYCF
{
	ToolkitFactory::ToolkitFactory()
	{
		registerGenericToolkit<StripperModel>();
        registerGenericToolkit<FundingStripperModel>();
        registerGenericToolkit<FundingSpreadStripperModel>();
		registerGenericToolkit<FundingSpreadModel>();
        registerGenericToolkit<IndexSpreadStripperModel>();
		registerGenericToolkit<IndexBaseSpreadStripperModel>();
		registerGenericToolkit<FundingIndexSpreadStripperModel>();
		registerGenericToolkit<MultiTenorModel>();
		registerGenericToolkit<InflationModel>();
		registerGenericToolkit<CTDModel>();
	}
	
}