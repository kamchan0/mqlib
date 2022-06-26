/*****************************************************************************
    StructureInstrumentUtils

	This file contains utility functions for structure instrument handling.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __StructureInstrumentUtils_H__
#define __StructureInstrumentUtils_H__


//	IDeA
//	#include "DictYieldCurve.h"
//	#include "DataExtraction.h"

//	Standard
#include <string>


namespace FlexYCF
{
	/*
	//	Create a step (respectively  a turn) instrument from the index-th
	//	record from the specified table containing all step (resp. turn)
	//	instruments
	template<class T>
	T createStepOrTurnFromTableRecord(const date buildDate,
									  const LTQuant::GenericData& instrumentData,
									  const size_t index)
	{
		const double spread(IDeA::extract<double>(instrumentData, IDeA_KEY(getKeyName<T>(), SPREAD), index));
		const date date(IDeA::extract<date>(instrumentData, IDeA_KEY(getKeyName<T>(), DATE), index));
			
		return T(buildDate, spread, date);
	}
	*/


	bool isTurnsOrStepsOrBumpsName(const std::string& instrumentTypeName);


}
#endif	//	__StructureInstrumentUtils_H__