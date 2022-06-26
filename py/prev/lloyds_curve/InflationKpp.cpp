/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InflationKpp.h"
#include "InflationModel.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    string InflationKpp::getName()
    {
        return "InflationKpp";
    }

    InflationKppPtr InflationKpp::createInstance(const LTQuant::GenericDataPtr)
    {
        return InflationKppPtr(new InflationKpp);
    }

    bool InflationKpp::createKnotPoints(const CalibrationInstruments& instruments, 
                                        const BaseModelPtr baseModel)
    {
		const InflationModelPtr singleCurve(std::tr1::dynamic_pointer_cast<InflationModel>(baseModel));

        for(size_t i(0); i < instruments.size(); ++i)
        {
            KnotPoint kp(instruments[i]->getLastRelevantTime(), 200.0, false, instruments[i]);
            singleCurve->addKnotPoint(kp);
        }
      
        baseModel->update();

        return true;
    }


	std::string InflationOnLastPaymentDateKpp::getName()
	{
		 return "InflationOnLastPaymentDateKpp";
	}

	InflationOnLastPaymentDateKppPtr InflationOnLastPaymentDateKpp::createInstance( const LTQuant::GenericDataPtr )
	{
		 return InflationOnLastPaymentDateKppPtr(new InflationOnLastPaymentDateKpp);
	}

	bool InflationOnLastPaymentDateKpp::createKnotPoints( const CalibrationInstruments& instruments, const BaseModelPtr baseModel )
	{
		const InflationModelPtr singleCurve(std::tr1::dynamic_pointer_cast<InflationModel>(baseModel));

		for(size_t i(0); i < instruments.size(); ++i)
		{
			KnotPoint kp(ModuleDate::getYearsBetween(instruments[i]->getStartDate(),instruments[i]->getEndDate()), 200.0, false, instruments[i]);
			singleCurve->addKnotPoint(kp);			
		}

		baseModel->update();

		return true;
	}
}   // FlexYCF