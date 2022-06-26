/*****************************************************************************

    @Originator	
    
    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved

*****************************************************************************/

#include "stdafx.h"

// FlexYCF
#include "SingleCurveFixedKpp.h"
#include "Data/GenericData.h"
#include "KnotPointPlacementFactory.h"
#include "SingleCurveModel.h"
#include "SingleCurveDefaultKpp.h"

// IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"

// LTQuantCore
#include "dates\DateBuilder.h"

using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
	SingleCurveFixedKpp::SingleCurveFixedKpp( const GenericDataPtr& fixedKpTbl, const BaseKnotPointPlacementPtr& nestedKpp )
		: m_fixedKpTbl(fixedKpTbl), m_nestedKpp(nestedKpp)
	{};

	SingleCurveFixedKppPtr SingleCurveFixedKpp::createInstance( const LTQuant::GenericDataPtr masterTable )
	{
		const LTQuant::GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));

		LTQuant::GenericDataPtr modelParametersTable;
		const bool modelParamsFound(IDeA::permissive_extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE,FLEXYC_MODELPARAMETERS), modelParametersTable));

		BaseKnotPointPlacementPtr nestedKpp;
		LTQuant::GenericDataPtr fixedKpTbl;

		if (modelParamsFound)
		{
			LTQuant::GenericDataPtr kppParametersTable;
			if(IDeA::permissive_extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, KPP_PARAMETERS), kppParametersTable))
			{
				std::string nestedKppName;
				if (IDeA::permissive_extract<std::string>(*kppParametersTable, IDeA_KEY(KPP_PARAMETERS, NESTEDKPP), nestedKppName, string()))
					nestedKpp = KnotPointPlacementFactory::createKnotPointPlacement(nestedKppName, masterTable);
				else
					nestedKpp = KnotPointPlacementFactory::createKnotPointPlacement(SingleCurveDefaultKpp::getName(), masterTable);

				IDeA::permissive_extract<GenericDataPtr>(*kppParametersTable, IDeA_KEY(KPP_PARAMETERS, FIXEDKNOTPOINTS), fixedKpTbl);
			}
		}

		return SingleCurveFixedKppPtr(new SingleCurveFixedKpp(fixedKpTbl, nestedKpp));
	}

	std::string SingleCurveFixedKpp::getName()
	{
		return "FixedKpp";
	}

	void SingleCurveFixedKpp::selectInstruments( CalibrationInstruments& instruments, const BaseModelPtr baseModel )
	{
		if (m_nestedKpp)
			m_nestedKpp->selectInstruments(instruments, baseModel);
	}

	bool SingleCurveFixedKpp::createKnotPoints( const CalibrationInstruments& instruments, const BaseModelPtr baseModel )
	{
		if (m_nestedKpp)
			m_nestedKpp->createKnotPoints(instruments, baseModel);

		if (m_fixedKpTbl)
		{
			const LT::TablePtr& fixedKpTbl = m_fixedKpTbl->table;
			
			const LT::date& startDate = baseModel->getValueDate();
			double x;
			const size_t tenorCol	= getColumnIndexFromTable(*fixedKpTbl, IDeA_KEY(FIXEDKNOTPOINTS, TENOR));
			const size_t calCol		= getColumnIndexFromTable(*fixedKpTbl, IDeA_KEY(FIXEDKNOTPOINTS, CALENDAR));
			const size_t xValCol	= getColumnIndexFromTable(*fixedKpTbl, IDeA_KEY(FIXEDKNOTPOINTS, X_VALUE));
			const size_t yValCol	= getColumnIndexFromTable(*fixedKpTbl, IDeA_KEY(FIXEDKNOTPOINTS, KNOTPOINTVALUE));

			SingleCurveModelPtr model = std::tr1::dynamic_pointer_cast<SingleCurveModel>(baseModel);

			// !XOR(has tenor, has x value)
			if (bool(tenorCol != LT::Table::not_found) == bool(xValCol != LT::Table::not_found))
				LT_THROW_ERROR( "Either tenor or x-value, but not both, must be provided, if FixedKpp table exists when using the Fixed Knot Point Placement method" );
			if (yValCol == LT::Table::not_found)
				LT_THROW_ERROR( "knot point value is compulsory, if FixedKpp table exists when using the Fixed Knot Point Placement method" );

			for (size_t i = 0; i < fixedKpTbl->rowsGet()-1; ++i)
			{
				if (tenorCol != LT::Table::not_found)
				{
					LT::date endDate;
					const LT::Str& tenor = fixedKpTbl->at(i+1, tenorCol);
					const LT::Str& calendar = (calCol ==  LT::Table::not_found) ? "" : fixedKpTbl->at(i+1, calCol);

					endDate = LTQC::DateBuilder::dateAdd(startDate, tenor, calendar).getAsLTdate();
					x = ModuleDate::getYearsBetween(startDate, endDate);
				}
				else
					x = fixedKpTbl->at(i+1, xValCol);

				model->addKnotPoint(KnotPoint(x, fixedKpTbl->at(i+1, yValCol), true));
			}
		}
		return true;
	}
}