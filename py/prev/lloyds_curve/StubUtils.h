/*****************************************************************************

    StubUtils

	Contains the declaration of stub related utility functions


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#pragma once
#ifndef __StubUtils_H__
#define	__StubUtils_H__

//	LTQC
#include "utils/EnumBase.h"


namespace LTQC
{
	class Tenor;
}


namespace FlexYCF
{
	LTQC_ENUM_DECLARE_BEGIN(EndDateCalculationType)
		NotAdjusted,					//	default value
		FromAccrualEndDate,
		FromStartDatePlusTenor,
		UseLocal
	LTQC_ENUM_DECLARE_END(EndDateCalculationType)
    
	bool shouldLegHaveStub(const LTQC::Tenor& maturityTenor,
						   const LTQC::Tenor& frequencyTenor);
	/*
	// Returns the appropriate end date calculation type given the maturity of a leg,
	//	its Tenor, and the applicable back stub en date calculation type
	EndDateCalculationType::Enum_t getBackStubEndDateCalculationType(const LT::Str& maturity,
																	 const LT::Str& tenor,
																	 const EndDateCalculationType::Enum_t applicableBackStubEndDateCalculationType);
	*/


}

#endif	//	__StubUtils_H__