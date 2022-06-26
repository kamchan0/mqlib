/*****************************************************************************
    StructureInstrumentCurveUtils

	This file contains utility functions for structure instrument curves


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __StructureInstrumentCurveUtils_H__
#define __StructureInstrumentCurveUtils_H__

//	Standard
#include <functional>

//	FlexYCF
#include "BumpTurnBase.h"


namespace FlexYCF
{
	//	Returns the derivative (relative the rate) of a bump or turn instrument
	inline double bumpOrTurnRateDerivative(const double time, const BumpTurnBase& bumpOrTurn)
	{
		// return std::min(time, bumpOrTurn.endTime()) - std::min(time, bumpOrTurn.startTime()); 
		return (time > bumpOrTurn.startTime()? std::min(time, bumpOrTurn.endTime()) - bumpOrTurn.startTime(): 0.0);
	}

	//	Helper functor that, given a structure instrument curve, 
	//	returns its associated structure instrument
	struct StructureInstrumentCurveToInstrument: public std::unary_function<IStructureInstrumentCurvePtr, StructureInstrument>
	{
	public:
		inline StructureInstrument operator()(const IStructureInstrumentCurvePtr& siCurve) const
		{
			return siCurve->getInstrument();
		}
	};

}

#endif	//	__StructureInstrumentCurveUtils_H__	