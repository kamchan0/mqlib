/*****************************************************************************
    StructureInstrumentCurve

	The StructureInstrumentCurve class is a generic implementation of
	the	IStructureInstrumentCurve that holds its related structure 
	instrument.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __StructureInstrumentCurve_H__
#define __StructureInstrumentCurve_H__

//	FleYCF
#include "IStructureInstrumentCurve.h"

//	LTQuantLib
#include "ModuleDate/InternalInterface/Utils.h"

//	IDeA
#include "DataExtraction.h"


namespace LTQuant
{
	class GDTblWrapper;
    typedef GDTblWrapper GenericData;
}


namespace FlexYCF
{

	//	Represents a single structure instrumente Curve, in
	//	the Log Fvf space
	//	
	//	The calculation of the derivative relative to its rate
	//	depends of the type of the structure instrument
	//	
	template<class T>
	class StructureInstrumentCurve: public IStructureInstrumentCurve
	{
	public:
		virtual double interpolate(const double time) const;
		virtual double rateDerivative(const double time) const;
		virtual void addSpineCurvePoints(SpineCurvePointVector& spineCurvePoints) const;
		
		//	Creation function use by StructureInstrumentCurveFactory
		static void loadStructureInstrumentCurves(const LT::date buildDate,
												  const LTQuant::GenericData& siData,
												  StructureInstrumentCurveList& siCurves);
		
		//	Create a single structure instrument curve from
		//	the related structure instrument
		static IStructureInstrumentCurvePtr createCurve(const T& structureInstrument)
		{
			return IStructureInstrumentCurvePtr(new StructureInstrumentCurve(structureInstrument));
		}

		//	Returns the instrument related to this curve
		virtual const T& getInstrument() const
		{
			return m_instrument;
		}

	private:
		explicit StructureInstrumentCurve(const T& structureInstrument):
			m_instrument(structureInstrument)
		{
		}

		T	m_instrument;
	};


	//
	//	Generic Implementation:
	//
	template<class T>
	double StructureInstrumentCurve<T>::interpolate(const double time) const
	{
		//	Note: structure instruments are linear in rate		
		return m_instrument.getRate() * rateDerivative(time);
	}

	template<class T>
	void StructureInstrumentCurve<T>::loadStructureInstrumentCurves(const LT::date buildDate,
																    const LTQuant::GenericData& siData,
																    StructureInstrumentCurveList& siCurves)
	{
		const size_t nbInstruments(IDeA::numberOfRecords(siData));

		for(size_t index(0); index < nbInstruments; ++index)
		{
			siCurves.push_back(createCurve(T::createFromTableRecord(buildDate, siData, index)));
		}
	}
	
}
#endif	//	__StructureInstrumentCurve_H__
