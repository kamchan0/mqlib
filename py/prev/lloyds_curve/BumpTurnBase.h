/*****************************************************************************
    BumpTurnBase

	The BumpTurnBase class is a helper class that encapsulates logic common
	to both Bump and Turn instruments.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __BumpTurnBase_H__
#define __BumpTurnBase_H__

//	FlexYCF
#include "StructureInstrument.h"


namespace FlexYCF
{

	class BumpTurnBase: public StructureInstrument
	{
	protected:
		BumpTurnBase(const LT::date buildDate,
					 const double spread,
					 const LT::date startDate,
					 const LT::date endDate,
					 const LT::Str& instrumentName,
					 const LT::Str& description):
			StructureInstrument(buildDate, spread, startDate, instrumentName, description),
			m_endDate(endDate),
			m_endTime(ModuleDate::getYearsBetweenAllowNegative(buildDate, endDate))
		{
		}
	
	public:
		inline LT::date startDate() const
		{
			return StructureInstrument::getDate();
		}

		inline double startTime() const
		{
			return StructureInstrument::getTime();
		}
		
		inline LT::date endDate() const
		{
			return m_endDate;
		}

		inline double endTime() const
		{
			return m_endTime;
		}

	private:
		LT::date	m_endDate;
		double	m_endTime;
	};
	
}


#endif	//	__BumpTurnBase_H__