/*****************************************************************************
    Step

	The Step class represents a step instrument.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __Step_H__
#define __Step_H__

//	FlexYCF
#include "StructureInstrument.h"
#include "StructureInstrumentUtils.h"
#include "FlexYcfUtils.h"


//	LTQuantLib
#include "ModuleDate/InternalInterface/Utils.h"


namespace LTQuant
{
	class GDTblWrapper; 
    typedef GDTblWrapper GenericData; 
}


namespace FlexYCF
{
	class Step: public StructureInstrument
	{
	public:
		explicit Step(const LT::date buildDate,
					  const double spread,
					  const LT::date stepDate):
			StructureInstrument(buildDate, spread, stepDate, "Step", date_to_string(stepDate))
		{
		}

		//	Static constructor
		//	Returns a Step instrument from a record in the table containing
		//	all the step instruments
		// inline 
			   static Step createFromTableRecord(const LT::date buildDate,
												 const LTQuant::GenericData& stepInstrumentData,
												 const size_t index);
		/*
		{
			return createStepOrTurnFromTableRecord<Step>(buildDate, stepInstrumentData, index);
		}
		*/

		inline LT::date stepDate() const
		{
			return StructureInstrument::getDate();
		}

		inline double stepTime() const
		{
			return StructureInstrument::getTime();
		}
	};

}

#endif	//	__Step_H__