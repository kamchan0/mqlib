/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "YieldCurvePointInitializer.h"
#include "CalibrationInstrument.h"


using namespace LTQC;

namespace FlexYCF
{
	YieldCurvePointInitializer::YieldCurvePointInitializer(BaseModel* const model):
		m_model(model)
	{
	}

	void YieldCurvePointInitializer::operator()(KnotPoint& knotPoint) const
	{
		CalibrationInstrumentPtr instrument(knotPoint.getRelatedInstrument());
		
		if(instrument)
		{
			knotPoint.y = instrument->getVariableInitialGuess(knotPoint.x, m_model);
		}
	}
	
}