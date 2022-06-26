/*****************************************************************************
    Todo: - Add source file description


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "StructureInstrumentUtils.h"


namespace FlexYCF
{

	bool isTurnsOrStepsOrBumpsName(const std::string& instrumentTypeName)
	{
		// TO DO: rewrite using IDeA key & aliases	
		return (instrumentTypeName == "Turns" || instrumentTypeName == "Steps" || instrumentTypeName == "Bumps");
	}
}