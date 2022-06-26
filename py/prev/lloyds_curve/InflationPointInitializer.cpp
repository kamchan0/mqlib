/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InflationPointInitializer.h"
#include "ILZCSwapInstrument.h"
#include "TransformFunction.h"
#include "ModuleDate/InternalInterface/Utils.h"


using namespace LTQC;

namespace FlexYCF
{
	void InflationPointInitializer::operator() (KnotPoint& knotPoint) const
	{
		CalibrationInstrumentPtr instrument(knotPoint.getRelatedInstrument());
		ILZCSwapInstrumentPtr inflSwap(std::tr1::dynamic_pointer_cast<ILZCSwapInstrument>(instrument));

		if(inflSwap)
		{
			const double time(knotPoint.x);

			// calculate the "adjusment factor" defined as "seasonality * structure" at end date
			const double adjFactor(m_inflationModel->getAdjustmentFactor(ModuleDate::getYearsBetween(m_inflationModel->getValueDate(), inflSwap->getEndDate())));

			// get the reference forward start index of the ILZC Swap
			const double index0(inflSwap->getForwardStartIndex(*m_inflationModel));
			
			// compute the knot-point y-value
			knotPoint.y = m_inflationModel->inflationIndexToVariable(index0 * inflSwap->getRateCoefficient() / adjFactor);

			LT_LOG << "Initialized point (" << time << ", " << knotPoint.y << ")" << std::endl;
		}
		else
		{
			// in this case the variables should be calibrated numerically
			// maybe we can track that
			LT_INFO << "one unknown knot-point has no related ZCILSwap attached" << std::endl;
		}
	}
}