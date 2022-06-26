/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

#include "DictYieldCurve.h"

#include "ModelSelection.h"
#include "CalibrationInstruments.h"

#include "ToolkitFactory.h"
#include "IToolkit.h"

#include "CashInstrument.h"
#include "ZeroRate.h"
#include "ZeroSpread.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "TenorBasisSwap.h"
#include "CrossCurrencySwap.h"
#include "ILZCSwapInstrument.h"
#include "FixedRateBond.h"

#include "StripperModel.h"
#include "FundingStripperModel.h"
#include "CTDModel.h"
#include "IndexStripperModel.h"
#include "MultiTenorModel.h"
#include "InflationModel.h"

#include "StripperNoCashEndDateKpp.h"
#include "MultiTenorStripperKpp.h"
#include "InflationKpp.h"

#include "Data/GenericData.h"


#include "BaseCurve.h"
#include "UkpCurve.h"

#include "ExtrapolationMethods.h"



using namespace LTQC;

namespace
{

	// Cache Parameters
	LTQuant::GenericDataPtr getDefaultCacheParameters()
	{
		LTQuant::GenericDataPtr cacheData(new LTQuant::GenericData("Cache Parameters", 0));

		cacheData->set<long>("Discount Factor Cache", 0, 1);
		cacheData->set<long>("Tenor DF Cache", 0, 1);
		cacheData->set<long>("Fwd Rate Cache", 0, 1);
		cacheData->set<long>("Discounted Fwd Rate Cache", 0, 1);
		cacheData->set<long>("Fxd Leg Cache", 0, 1);
		cacheData->set<long>("Flt Leg Cache", 0, 1);
	
		return cacheData;
	}

}


namespace FlexYCF
{
	void listOfAllowedModelsFromInstrumentList(const CalibrationInstruments& calibrationInstruments,
											   std::vector<std::string>& allowedModelNames)
	{
		allowedModelNames.clear();

		if(calibrationInstruments.has<ILZCSwapInstrument>())
		{
			allowedModelNames.push_back(InflationModel::getName());
		}
		else if(calibrationInstruments.has<CashInstrument>() || calibrationInstruments.has<Futures>() || calibrationInstruments.has<InterestRateSwap>() || calibrationInstruments.has<FixedRateBondInstrument>())
		{
			allowedModelNames.push_back(StripperModel::getName());

			if(calibrationInstruments.has<TenorBasisSwap>() || calibrationInstruments.has<CrossCurrencySwap>())
			{
				allowedModelNames.push_back(MultiTenorModel::getName());
			}
		}
        if(calibrationInstruments.has<CrossCurrencySwap>() || calibrationInstruments.has<ZeroRate>())
        {
            allowedModelNames.push_back(FundingStripperModel::getName());
            allowedModelNames.push_back(FundingSpreadStripperModel::getName());
            allowedModelNames.push_back(IndexSpreadStripperModel::getName());
			allowedModelNames.push_back(FundingSpreadModel::getName());
        }
		 
		if(calibrationInstruments.has<ZeroSpread>())
        {
            allowedModelNames.push_back(FundingSpreadStripperModel::getName());
			allowedModelNames.push_back(CTDModel::getName());
        }
	}

	std::string getDefaultModelFromInstrumentList(const CalibrationInstruments& calibrationInstruments)
	{
		if(calibrationInstruments.has<ILZCSwapInstrument>())
		{
			return InflationModel::getName();
		}
	
		if(calibrationInstruments.has<TenorBasisSwap>() || calibrationInstruments.has<CrossCurrencySwap>())
		{
			return MultiTenorModel::getName();		
		}
			
		return StripperModel::getName();
	}

	std::string getDefaultModelFromInstrumentListData(const LTQuant::GenericDataPtr& instrumentList)
	{
		if(instrumentList->doesTagExist("ILZCSwap"))
		{
			return InflationModel::getName();
		}
		else if(instrumentList->doesTagExist("Tenor Basis") || instrumentList->doesTagExist("Tenor Basis Swaps")
			|| instrumentList->doesTagExist("Ccy Basis"))
		{
			return MultiTenorModel::getName();
		}
	
		return StripperModel::getName();
	}

	LTQuant::GenericDataPtr getDefaultParametersFromModelName(const std::string& modelName,
															  const LTQuant::GenericDataPtr curveDetailsData,
															  const LTQuant::GenericDataPtr instrumentListData)
	{
		// Build the toolkit based on the name of the model
		// TO DO: if the model name is incorrect, use a default model? or guess based on the curve details?
		IToolkitPtr toolkit(ToolkitFactory::createToolkit(modelName, curveDetailsData, instrumentListData));
	
		// Return the model parameters table
		return toolkit->getModelParameters();
	}
}