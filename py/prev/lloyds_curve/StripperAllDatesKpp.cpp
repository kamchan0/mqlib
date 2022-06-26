/*****************************************************************************
    
	This file contains the implementation of StripperNoCashEndKpp
    
	
	@Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
//	FlexYCF
#include "StripperAllDatesKpp.h"
#include "BaseModel.h"
#include "StripperModel.h"
#include "CashInstrument.h"
#include "Futures.h"
#include "KnotPlacementUtils.h"

//	LTQuantLib
#include "DateUtils.h"

using namespace std;


using namespace LTQC;

namespace FlexYCF
{
    string StripperAllDatesKpp::getName()
    {
         return "All Dates";
    }

	void StripperAllDatesKpp::selectInstruments(CalibrationInstruments& instruments, 
												const BaseModelPtr baseModel)
	{
		selectInstrumentsForStripperModel(*baseModel, instruments);
	}

    bool StripperAllDatesKpp::createKnotPoints(const CalibrationInstruments& instruments, 
                                               const BaseModelPtr baseModel)
    {
		const StripperModelPtr stripperModel(std::tr1::dynamic_pointer_cast<StripperModel>(baseModel));
		const LT::date valueDate(stripperModel->getValueDate());
		const double defaultInitRate(0.05); //  5%, could be a param

        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
			// Always add the end date of the instrument
			const double endDateFlow(ModuleDate::getYearsBetween(valueDate, (*iter)->getEndDate()));
			stripperModel->addKnotPoint(KnotPoint(endDateFlow, endDateFlow * defaultInitRate, false, *iter));
		}
		return true;
    }

}   //  FlexYCF
