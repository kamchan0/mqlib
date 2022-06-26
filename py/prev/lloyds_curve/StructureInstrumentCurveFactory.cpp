/*****************************************************************************
    Todo: - Add source file description


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"


//	FlexYCF
#include "StructureInstrumentCurveFactory.h"
#include "StructureInstrumentCurve.h"
#include "StructureInstrument.h"

#include "Step.h"
#include "Bump.h"
#include "Turn.h"


using namespace LTQC;

namespace FlexYCF
{
	void StructureInstrumentCurveFactory::loadStructureInstrumentCurves(const std::string& instrumentTypeName,
																		const LT::date buildDate,
																	    const LTQuant::GenericData& instrumentListData,
																	    StructureInstrumentCurveList& siCurves)
	{
		siCurves.clear();

		//	Same key creation as in NameFactory
		//
		//	Note: ideally this fundtion should be encapsulated and accessible
		//	at least do derived factories, but this is not done so in NameFactory
		size_t startpos = instrumentTypeName.find_first_not_of(" \t\n\v\f\r");
		size_t endpos = instrumentTypeName.find_last_not_of(" \t\n\v\f\r");
		std::string instrumentTypeKey(startpos == std::string::npos ? std::string("") : instrumentTypeName.substr( startpos, endpos-startpos+1 ));
		std::transform(instrumentTypeKey.begin(), instrumentTypeKey.end(), instrumentTypeKey.begin(), ::toupper);
		
		//	Skip the non-structure instrument types
		if(instance()->m_objects.find(instrumentTypeKey) != instance()->m_objects.end())
		{
			StructureInstrumentCurveFactory::create(instrumentTypeKey)(buildDate, instrumentListData, siCurves);
		}
	}
	
	StructureInstrumentCurveFactory::StructureInstrumentCurveFactory()
	{
		registerStructureInstrumentCurve<Step>();
		registerStructureInstrumentCurve<Bump>();
		registerStructureInstrumentCurve<Turn>();
	}

	StructureInstrumentCurveFactory* const StructureInstrumentCurveFactory::instance()
	{
		static StructureInstrumentCurveFactory structureInstrumentCurveFactory;
		return &structureInstrumentCurveFactory;
	}
}