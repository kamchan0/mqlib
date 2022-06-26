/*****************************************************************************
    Turn

	The Turn class represents a turn instrument in 
	the structure curve.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __Turn_H__
#define __Turn_H__


//	FlexYCF
#include "BumpTurnBase.h"
#include "StructureInstrumentUtils.h"
#include "FlexYcfUtils.h"


namespace LTQuant
{
    class GDTblWrapper; 
    typedef GDTblWrapper GenericData; 
}

namespace FlexYCF
{
	//	John Adcock's comment:
	// a turn is modeled as a step up on the start date and a step 
    // down a day later
    // note that turns are quoted as change
    // in futures quote rather than as an increased overnight rate
    // and so we need to multiply by 91 to get it in the correct form
	inline double turnSpreadMultiplier()
	{
		return 91.0;
	}


	class Turn: public BumpTurnBase
	{
	public:
        Turn(const LT::date buildDate,
            const double spread,
            const LT::date turnDate):
        BumpTurnBase(buildDate, spread * turnSpreadMultiplier(), turnDate, turnDate + 1, "Turn", date_to_string(turnDate))
        {
		}
		
		//	Static constructor
		//	Returns a Step instrument from a record in the table containing
		//	all the step instruments
		//inline 
			   static Turn createFromTableRecord(const LT::date buildDate,
												 const LTQuant::GenericData& stepInstrumentData,
												 const size_t index);
		/*
		{
			return createStepOrTurnFromTableRecord<Turn>(buildDate, stepInstrumentData, index);
		}
		*/
	};
	
}
#endif	//	__Turn_H__