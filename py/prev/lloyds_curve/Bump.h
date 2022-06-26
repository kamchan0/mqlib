/*****************************************************************************
    Bump

	The Bump class represents a bump instrument.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __Bump_H__
#define __Bump_H__


//	Standard
#include <sstream>

//	FlexYCF
#include "BumpTurnBase.h"
#include "FlexYcfUtils.h"


namespace LTQuant
{
	class GDTblWrapper;
    typedef GDTblWrapper GenericData;
}

namespace FlexYCF
{
	class Bump: public BumpTurnBase
	{
	public:
		Bump(const LT::date buildDate,
			 const double spread,
			 const LT::date startDate,
			 const LT::date endDate):
			BumpTurnBase(buildDate, spread, startDate, endDate, "Bump", date_to_string(startDate) + " - " + date_to_string(endDate))
		{
		}

		//	Static constructor
		//	Returns a Bump instrument from the index-th record in the specified 
		//	table containing all the bump instruments
		static Bump createFromTableRecord(const LT::date buildDate,
										  const LTQuant::GenericData& bumpInstrumentData,
										  const size_t index);
	};

}

#endif	//	__Bump_H__