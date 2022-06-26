/*****************************************************************************
    IStructureInstrumentCurve

	IStructureInstrumentCurve is the interface for Structure instrument
	curves in a logFvf formulation


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __IStructureInstrumentCurve_H__
#define __IStructureInstrumentCurve_H__

//	LTQC
#include "Macros.h"

//	Standard
#include <vector>

//	FlexYCF
#include "SpineCurvePoint.h"


namespace FlexYCF
{
	class StructureInstrument;


	//	Interface for the logFvf representation of a structure instrument
	//	curve
	class IStructureInstrumentCurve	//: public ICurve/CurveFormulation ??
	{
	public:
		virtual ~IStructureInstrumentCurve() = 0 {}
		
		//	Interpolates (or extrapolates) on the curve
		virtual double interpolate(const double time) const	= 0;
	
		//	Calculates the derivative relative to the rate of the 
		//	associated structure instrument
		virtual double rateDerivative(const double time) const = 0;

		//	Adds the spine curve point(s) related to the 		
		//	instrument of a structure instrument curve
		virtual void addSpineCurvePoints(SpineCurvePointVector& spineCurvePoints) const = 0;

		//	Returns a (const reference to) the associated structure instrument
		//	relatde to this instrument curve
		virtual const StructureInstrument& getInstrument() const = 0;
	};

	LTQC_DECLARE_SMART_PTRS( IStructureInstrumentCurve )

	typedef	std::vector<IStructureInstrumentCurvePtr> StructureInstrumentCurveList;
}

#endif	// __IStructureInstrumentCurve_H__