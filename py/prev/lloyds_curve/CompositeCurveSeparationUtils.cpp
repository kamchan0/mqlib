/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ICurve.h"
#include "BaseCurve.h"
#include "InterpolationCurve.h"
#include "CompositeCurve.h"
#include "CurveFormulation.h"
#include "Futures.h"

#include "ModuleDate/InternalInterface/Utils.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
	void setSeparationPoint(const LT::date valueDate, const BaseCurve& baseCurve)
	{
		const CompositeCurvePtr compositeCurve(std::tr1::dynamic_pointer_cast<CompositeCurve>(baseCurve.m_interpolationCurve));

		// If the interpolation curve of the curve is a composite curve whose separation x has been set by default
		if(compositeCurve)
		{	
			switch(compositeCurve->getSeparationType())
			{
			case IDeA::CompositeSeparationType::Assigned:
				break;	// do nothing
			case IDeA::CompositeSeparationType::Default:
			case IDeA::CompositeSeparationType::LastFutureMaturity:
				{
					// find last future end date

					//	Otherwise, try to find the last future's maturity ...
					double lastFutureMaturity(-1.0);

					for(KnotPoints::const_iterator iter(compositeCurve->begin()); iter != compositeCurve->end(); ++iter)
					{
						if(iter->getRelatedInstrument() && iter->getRelatedInstrument()->getType() == Futures::getName())
						{
							lastFutureMaturity = max(lastFutureMaturity, ModuleDate::getYearsBetween(valueDate, iter->getRelatedInstrument()->getEndDate()));
						}
					}

					//	.. and use it a separation it if exists, otherwise leave the default value
					if(lastFutureMaturity > 0.0)
					{
						// found? set composite curve separation point:
						compositeCurve->setSeparationX(lastFutureMaturity);
					}

					break;
				}
			case IDeA::CompositeSeparationType::LastFixedKnotPoint:
				{
					KnotPoints::const_iterator iter = compositeCurve->end();
					do { --iter; } while(!iter->isKnown && iter != compositeCurve->begin());
					compositeCurve->setSeparationX(iter->x);
					break;
				}
			}

		}
	}

	void setSeparationPoint(const LT::date valueDate, const CurveFormulation& curveFormulation)
	{
		if(curveFormulation.m_baseCurve)
		{
			setSeparationPoint(valueDate, *(curveFormulation.m_baseCurve));
		}
	}
}