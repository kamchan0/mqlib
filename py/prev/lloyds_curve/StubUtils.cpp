/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "StubUtils.h"

//	LTQC
#include "Tenor.h"


LTQC_ENUM_DEFINE_BEGIN(FlexYCF::EndDateCalculationType)
	LTQC_REGISTER_ENUM(	NotAdjusted				, "NotAdjusted"					);
	LTQC_REGISTER_ENUM(	FromAccrualEndDate		, "FromAccrualEndDate"			);
	LTQC_REGISTER_ALIAS(FromAccrualEndDate		, "From Accrual End Date"		);
	LTQC_REGISTER_ENUM(	FromStartDatePlusTenor	, "FromStartDatePlusTenor"		);
	LTQC_REGISTER_ALIAS(FromStartDatePlusTenor	, "From Start Date Plus Tenor"	);
	LTQC_REGISTER_ENUM(	UseLocal			    , "UseLocal"   					);
	LTQC_REGISTER_ALIAS(UseLocal			    , "NotValid"   					);
LTQC_ENUM_DEFINE_END(FlexYCF::EndDateCalculationType)


using namespace LTQC;

namespace FlexYCF
{
	bool shouldLegHaveStub(const LTQC::Tenor& maturityTenor,
						   const LTQC::Tenor& frequencyTenor)
	{
		//	Checks if there is stub period. This works for leg tenors of at least 1M
		return maturityTenor < frequencyTenor || maturityTenor.asMonths() % frequencyTenor.asMonths() != 0;
	}

}