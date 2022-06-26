/*****************************************************************************
    StructureInstrument

	The StructureInstrument class is the base class for structure
	instruments.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __StructureInstrument_H__
#define __StructureInstrument_H__


//	LTQuantLib
#include "ModuleDate/InternalInterface/Utils.h"

//	FlexYCF
#include "FlexYcfUtils.h"
#include "RiskInstrument.h"

namespace FlexYCF
{
	
	//
	//	The base class for all structure instruments
	//
	//	Note: this class is *not* meant to be used polymorphically
	//
	class StructureInstrument: public RiskInstrument
	{
	protected:
		StructureInstrument(const LT::date buildDate,
							const double spread,
							const LT::date date_,
							const LT::Str& instrumentName,
							const LT::Str& description):
			RiskInstrument(spread, instrumentName, description),
			m_date(date_),
            m_time(
            ModuleDate::getYearsBetweenAllowNegative(buildDate, date_)
            )
		{
		}	

	public:
		~StructureInstrument() 
		{
		}
		
	protected:
		inline LT::date getDate() const
		{
			return m_date;
		}
		
		inline double getTime() const
		{
			return m_time;
		}

	private:
		LT::date	m_date;
		double	m_time;
	};

	DEFINE_VECTOR( StructureInstrument )
}
#endif	//	__StructureInstrument_H__