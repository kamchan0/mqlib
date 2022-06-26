/*****************************************************************************

	ResidualsUtils

	Contains the declaration of residuals related utility functions


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#pragma once
#ifndef __ResidualsUtils_H__
#define	__ResidualsUtils_H__

//	LTQC
#include "utils/EnumBase.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"


namespace FlexYCF
{
	LTQC_ENUM_DECLARE_BEGIN(LeastSquaresRepresentationType)
		PV,
		Rate
	LTQC_ENUM_DECLARE_END(LeastSquaresRepresentationType)

	//	Returns the default Least Squares representation type
	LeastSquaresRepresentationType::Enum_t defaultLeastSquaresRepresentationType();

	template<class T>
	LeastSquaresRepresentationType::Enum_t permissiveGetLSRepresentationTypeFromSolverParamsTable(const T& solverParamsTable)
	{
		std::string representationTypeStr;
		IDeA::permissive_extract<string>(solverParamsTable, IDeA_KEY(SOLVERPARAMETERS, LSRTYPE), representationTypeStr, 
										 LeastSquaresRepresentationType::toString(defaultLeastSquaresRepresentationType()).data());
		return LeastSquaresRepresentationType::fromString(representationTypeStr);
	}
}

#endif	//	__ResidualsUtils_H__