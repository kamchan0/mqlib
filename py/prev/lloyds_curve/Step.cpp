/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "FlexYcfUtils.h"
#include "Step.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"


using namespace LTQC;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<Step>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, STEPS);
	}

	//	see createStepOrTurnFormTableRecord in StructureInstrumentUtils.h
	Step Step::createFromTableRecord(const LT::date buildDate,
									 const LTQuant::GenericData& stepInstrumentData,
									 const size_t index)
	{
		//	Same as spread
		const double spread(IDeA::extract<double>(stepInstrumentData, IDeA_KEY(STEP, SPREAD), index));
		const LT::date stepDate(IDeA::extract<LT::date>(stepInstrumentData, IDeA_KEY(STEP, DATE), index));
			
		return Step(buildDate, spread, stepDate);
	}
	
}