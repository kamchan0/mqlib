/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "FlexYcfUtils.h"
#include "Turn.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"

using namespace LTQC;

namespace FlexYCF
{
	
	template<>
	const IDeA::DictionaryKey& getKey<Turn>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, TURNS);
	}

	//	see createStepOrTurnFormTableRecord in StructureInstrumentUtils.h
	Turn Turn::createFromTableRecord(const LT::date buildDate,
									 const LTQuant::GenericData& bumpInstrumentData,
								     const size_t index)
	{
		// Same as Step
		const double spread(IDeA::extract<double>(bumpInstrumentData, IDeA_KEY(TURN, SPREAD), index));
		const LT::date turnDate(IDeA::extract<LT::date>(bumpInstrumentData, IDeA_KEY(TURN, DATE), index));

		return Turn(buildDate, spread, turnDate);
	}
	
}