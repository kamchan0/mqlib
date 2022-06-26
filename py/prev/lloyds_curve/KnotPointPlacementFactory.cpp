/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "KnotPointPlacementFactory.h"
#include "SingleCurveDefaultKpp.h"
#include "SingleCurveBestFitKpp.h"
#include "SingleCurveFixedKpp.h"
#include "MultiTenorDefaultKpp.h"
#include "MultiTenorStripperKpp.h"
#include "StripperAllDatesKpp.h"
#include "StripperNoCashEndDateKpp.h"
#include "InflationKpp.h"
#include "NaturalSwapTenorKpp.h"
#include "CommodityDefaultKpp.h"
#include "IndexFundingStripperKpp.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{   
    KnotPointPlacementFactory::KnotPointPlacementFactory()
    {
        registerKnotPointPlacement<MultiTenorDefaultKpp>();
		registerKnotPointPlacement<SingleCurveBestFitKpp>();
		registerKnotPointPlacement<SingleCurveFixedKpp>();
        registerKnotPointPlacement<SingleCurveDefaultKpp>();
        registerKnotPointPlacement<StripperAllDatesKpp>(); 
        registerKnotPointPlacement<StripperNoCashEndDateKpp>();
        registerKnotPointPlacement<InflationKpp>();
		registerKnotPointPlacement<InflationOnLastPaymentDateKpp>();
        registerKnotPointPlacement<MultiTenorStripperKpp>();
		registerKnotPointPlacement<NaturalSwapTenorKpp>();
        registerKnotPointPlacement<CommodityDefaultKpp>();
        registerKnotPointPlacement<IndexFundingStripperKpp>();
    }

	BaseKnotPointPlacementPtr KnotPointPlacementFactory::createKnotPointPlacement(const string& objectId,
                                                                                  const LTQuant::GenericDataPtr masterTable)
    {
        return KnotPointPlacementFactory::create(objectId)(masterTable);
    }

    KnotPointPlacementFactory* const KnotPointPlacementFactory::instance()
    {
        static KnotPointPlacementFactory knotPointPlacementFactory;
        return &knotPointPlacementFactory;
    }
}   // FlexYCF