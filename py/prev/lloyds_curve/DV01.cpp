/*****************************************************************************
    
	DV01

	This file contains the implementation of function to calculate
	the DV01s of input instruments of a FlexYCF model.


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "DV01.h"
#include "BaseModel.h"
#include "CalibrationInstrument.h"
#include "CalibrationInstruments.h"
#include "FlexYcfUtils.h"


using namespace LTQC;

namespace FlexYCF
{
	//	helper functor to calculate DV01 of input instruments
	struct DV01Calculator: public std::unary_function<CachedDerivInstrumentPtr, InstrumentDelta>
	{
	public:
	    DV01Calculator()
		{
		}
		
		inline InstrumentDelta operator()(const CachedDerivInstrumentConstPtr& instrument)
		{
			return InstrumentDelta(*instrument, 
								   instrument->wasPlaced()? instrument->getBPV(): 0.0,
                                   instrument->wasPlaced()? 1.0 : 0.0,
								   getDeltaType(*instrument));
		}
	};


	void calculateDV01s(const BaseModel& model,
						InstrumentDeltaVector& dv01s)
	{
		CachedDerivInstruments fullInstruments(model.getFullPrecomputedInstruments());	
		dv01s.resize(fullInstruments.size());

		std::transform(fullInstruments.begin(), fullInstruments.end(), dv01s.begin(), DV01Calculator());
	}

}