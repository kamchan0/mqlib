/*****************************************************************************
    StructureInstrumentCurve.cpp

	This file contains implementation of the structure instrument curves
	for each structure instrument type


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "StructureInstrumentCurve.h"
#include "StructureInstrument.h"
#include "StructureInstrumentCurveUtils.h"
#include "Step.h"
#include "Bump.h"
#include "Turn.h"
#include "FlexYcfUtils.h"
#include "SpineCurvePoint.h"

//	LTQuantLib
#include "Data/GenericData.h"
#include "ModuleDate/InternalInterface/Utils.h"

//	IDeA
#include "DataExtraction.h"
#include "DictionaryManager.h"
#include "DictYieldCurve.h"


using namespace LTQC;

namespace FlexYCF
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Step
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<>
	double StructureInstrumentCurve<Step>::rateDerivative(const double time) const
	{
		return (time > m_instrument.stepTime()? time - m_instrument.stepTime(): 0.0);
	}

	template<>
	void StructureInstrumentCurve<Step>::addSpineCurvePoints(SpineCurvePointVector& spineCurvePoints) const
	{
		spineCurvePoints.push_back(SpineCurvePoint(m_instrument.stepTime(),		//	x
												   m_instrument.getRate(),		//	y
												   true,						//	fixed?
												   getKeyName<Step>(),			//	description
												   m_instrument.stepDate()));	//	maturity
	}
		

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Bump
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<>
	double StructureInstrumentCurve<Bump>::rateDerivative(const double time) const
	{
		return bumpOrTurnRateDerivative(time, m_instrument);
	}

	template<>
	void StructureInstrumentCurve<Bump>::addSpineCurvePoints(SpineCurvePointVector& spineCurvePoints) const
	{
		//	Note: count the bump to clearly identify the in the spine
		//	curve in case some of them overlap
		static int bumpCount(0);
		++bumpCount;

		spineCurvePoints.push_back(SpineCurvePoint(m_instrument.startTime(),						//	x
												   m_instrument.getRate(),							//	y
												   true,											//	fixed?
												   getKeyName<Bump>() + to_string(bumpCount),		//	description
												   m_instrument.startDate()));						//	maturity	
	

		spineCurvePoints.push_back(SpineCurvePoint(m_instrument.endTime(),							//	x
												   -m_instrument.getRate(),							//	y
												   true,											//	fixed?
												   getKeyName<Bump>() + to_string(bumpCount),		//	description
												   m_instrument.endDate()));						//	maturity
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Turn
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template<>
	double StructureInstrumentCurve<Turn>::rateDerivative(const double time) const
	{
		return bumpOrTurnRateDerivative(time, m_instrument);
	}

	template<>
	void StructureInstrumentCurve<Turn>::addSpineCurvePoints(SpineCurvePointVector& spineCurvePoints) const
	{
		spineCurvePoints.push_back(SpineCurvePoint(m_instrument.startTime(),	//	x
												   m_instrument.getRate(),		//	y
												   true,						//	fixed?
												   getKeyName<Turn>(),			//	description
												   m_instrument.startDate()));	//	maturity
	}
}