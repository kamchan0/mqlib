/*****************************************************************************
    
	This file contains the implementation of StripperNoCashEndKpp
    
	
	@Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "StripperNoCashEndDateKpp.h"
#include "CalibrationInstruments.h"
#include "CashInstrument.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "StripperModel.h"

#include "DateUtils.h"
#include "Data\GenericData.h"
#include "ModuleDate/InternalInterface/DayCounterFactory.h"
//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"





using namespace std;

using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
   

    StripperNoCashEndDateKpp::StripperNoCashEndDateKpp(const LT::date valueDate, CashSelection cashSelection) :
        m_valueDate(valueDate), m_cashSelection(cashSelection)
    {
    }

    string StripperNoCashEndDateKpp::getName()
    {
        return "No Cash End Date";
    }
    
    StripperNoCashEndDateKppPtr StripperNoCashEndDateKpp::createInstance(const LTQuant::GenericDataPtr masterTable)
    {
        
		const LTQuant::GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        LTQuant::GenericDataPtr modelParametersTable;
        const bool modelParamsFound(IDeA::permissive_extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE,FLEXYC_MODELPARAMETERS), modelParametersTable));
       
        CashSelection cashSelection(CashSelection::BaseRate);
        if (modelParamsFound) 
		{
			LTQuant::GenericDataPtr kppParametersTable;
			const bool kppParamsFound(IDeA::permissive_extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, KPP_PARAMETERS), kppParametersTable));
			if(kppParamsFound)
			{
                std::string cashSelectionRuleStr;
			    IDeA::permissive_extract<std::string>(*kppParametersTable, IDeA_KEY(KPP_PARAMETERS, CASHSELECTION), cashSelectionRuleStr);
			    if(!cashSelectionRuleStr.empty())
			    {
				    cashSelection = CashSelection::fromString(cashSelectionRuleStr);
			    }
			}
		}

        return StripperNoCashEndDateKppPtr(new StripperNoCashEndDateKpp(valueDate, cashSelection));
    }
    
    void StripperNoCashEndDateKpp::selectInstruments(CalibrationInstruments& instruments,
                                                     const BaseModelPtr baseModel)
    {
		 selectInstrumentsForStripperModel(*baseModel, instruments, m_cashSelection);
    }

    bool StripperNoCashEndDateKpp::createKnotPoints(const CalibrationInstruments& instruments, 
                                                    const BaseModelPtr baseModel)
    {
        // dump the knot-points to the model, using an initial flat rate
		const StripperModelPtr stripperModel(std::tr1::dynamic_pointer_cast<StripperModel>(baseModel));
        const double modelTenor(stripperModel->getTenor());
        const double defaultInitRate(0.05); //  5%, could be a param

        const LT::date futuresOrFRAStartDate( max( getFirstFuturesStartDate(m_valueDate, instruments), getFirstFRAStartDate(m_valueDate, instruments) ) );

        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
            // remember the cash instrument with same tenor as the model's
            // need to know the first futures before processing
			const CashInstrumentPtr tmpCashInstrument(std::tr1::dynamic_pointer_cast<CashInstrument>(*iter));
            double dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
            
            
            if( tmpCashInstrument && futuresOrFRAStartDate > m_valueDate )
            {
                if( CashSelection::BaseRate == m_cashSelection )
                {
                    if( tmpCashInstrument->getTenor() == modelTenor  )
                    {
                        dateFlow = ModuleDate::getYearsBetween(m_valueDate, futuresOrFRAStartDate);
                    }
                }
                else if( CashSelection::FutureStartBlending == m_cashSelection )
                {
                    if( tmpCashInstrument->getEndDate() >= futuresOrFRAStartDate )
                    {
                        dateFlow = ModuleDate::getYearsBetween(m_valueDate, futuresOrFRAStartDate);
                    }
                }
                else if( CashSelection::FutureStartLinear == m_cashSelection )
                {
                    if( tmpCashInstrument->getEndDate() >= futuresOrFRAStartDate )
                    {
                       baseModel->JacobianIsNotSupported();
                    }
                }
            }
            stripperModel->addKnotPoint(KnotPoint(dateFlow, dateFlow * defaultInitRate, false, *iter));
        }

        return true;
    }

}   //  FlexYCF