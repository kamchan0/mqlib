/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "FlexYcfUtils.h"
#include "Bump.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"


using namespace LTQC;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<Bump>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, BUMPS);
	}
	
	/*
	Bump::Bump(const date buildDate,
		 const double spread,
		 const date startDate,
		 const date endDate):
		BumpTurnBase(buildDate, spread, startDate, endDate, "Bump")
	{
		setDescription(date_to_string(startDate) + " - " + date_to_string(endDate));
	}
	*/

	Bump Bump::createFromTableRecord(const LT::date buildDate,
									 const LTQuant::GenericData& bumpInstrumentData,
								     const size_t index)
	{
		const double spread(IDeA::extract<double>(bumpInstrumentData, IDeA_KEY(BUMP, SPREAD), index));
		const LT::date startDate(IDeA::extract<LT::date>(bumpInstrumentData, IDeA_KEY(BUMP, STARTDATE), index));
		const LT::date endDate(IDeA::extract<LT::date>(bumpInstrumentData, IDeA_KEY(BUMP, ENDDATE), index));

		return Bump(buildDate, spread, startDate, endDate);
	}

}