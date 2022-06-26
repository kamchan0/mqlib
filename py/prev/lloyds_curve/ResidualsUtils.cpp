/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ResidualsUtils.h"


LTQC_ENUM_DEFINE_BEGIN(FlexYCF::LeastSquaresRepresentationType)
	LTQC_REGISTER_ENUM(	PV	, "PV"		);
	LTQC_REGISTER_ENUM( Rate, "Rate"	);
LTQC_ENUM_DEFINE_END(FlexYCF::LeastSquaresRepresentationType)



using namespace LTQC;

namespace FlexYCF
{
	LeastSquaresRepresentationType::Enum_t defaultLeastSquaresRepresentationType()
	{
		return LeastSquaresRepresentationType::PV;
	}
}
