#include "stdafx.h"

// FlexYCF
#include "SingleCurveBestFitKpp.h"
#include "LeastSquaresResiduals.h"
#include "Data/GenericData.h"
#include "KnotPointPlacementFactory.h"

#include "StripperModel.h"
#include "InflationModel.h"

// IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"

// STD
#include <functional>

using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{
	SingleCurveBestFitKpp::SingleCurveBestFitKpp(	const GenericDataPtr& knotPointInstrumentTbl, 
													const GenericDataPtr& extraKpTbl, 
													const GenericDataPtr& weightedInstrumentTbl,
													const BaseKnotPointPlacementPtr& nestedKpp)
		:	m_knotPointInstrumentTbl(knotPointInstrumentTbl), m_extraKpTbl(extraKpTbl), 
			m_weightedInstrumentTbl(weightedInstrumentTbl), m_nestedKpp(nestedKpp)
	{};

	SingleCurveBestFitKppPtr SingleCurveBestFitKpp::createInstance( const LTQuant::GenericDataPtr masterTable)
	{
		const LTQuant::GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));

		LTQuant::GenericDataPtr modelParametersTable;
		const bool modelParamsFound(IDeA::permissive_extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE,FLEXYC_MODELPARAMETERS), modelParametersTable));

		BaseKnotPointPlacementPtr nestedKpp;
		LTQuant::GenericDataPtr knotPointInstrumentTbl, extraKpTbl, weightedInstrumentTbl;

		if (modelParamsFound)
		{
			LTQuant::GenericDataPtr kppParametersTable;
			if(IDeA::permissive_extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, KPP_PARAMETERS), kppParametersTable))
			{
				std::string nestedKppName;
				if (IDeA::permissive_extract<std::string>(*kppParametersTable, IDeA_KEY(KPP_PARAMETERS, NESTEDKPP), nestedKppName, string()))
					nestedKpp = KnotPointPlacementFactory::createKnotPointPlacement(nestedKppName, masterTable);

				IDeA::permissive_extract<GenericDataPtr>(*kppParametersTable, IDeA_KEY(KPP_PARAMETERS, KNOTPOINTINSTRUMENTS), knotPointInstrumentTbl);
				IDeA::permissive_extract<GenericDataPtr>(*kppParametersTable, IDeA_KEY(KPP_PARAMETERS, EXTRAKNOTPOINTS), extraKpTbl);
				IDeA::permissive_extract<GenericDataPtr>(*kppParametersTable, IDeA_KEY(KPP_PARAMETERS, WEIGHTEDINSTRUMENTS), weightedInstrumentTbl);
			}
		}

		return SingleCurveBestFitKppPtr(new SingleCurveBestFitKpp(knotPointInstrumentTbl,extraKpTbl, weightedInstrumentTbl, nestedKpp));
	}

	std::string SingleCurveBestFitKpp::getName()
	{
		return "Best Fit";
	}

	void SingleCurveBestFitKpp::selectInstruments( CalibrationInstruments& instruments, const BaseModelPtr baseModel )
	{
		if (m_nestedKpp)
			m_nestedKpp->selectInstruments(instruments, baseModel);

		if (m_weightedInstrumentTbl)
		{			
			const LT::TablePtr& weightedInstrumentTbl = m_weightedInstrumentTbl->table;	
			LT::Str instrumentTblID, instrumentID;

			for (size_t i = 0; i < weightedInstrumentTbl->rowsGet()-1; ++i)
			{	
				const LT::Str instrumentType = IDeA::extract<LT::Str>(*weightedInstrumentTbl,  IDeA_KEY(WEIGHTEDINSTRUMENTS, TYPE), i);

				for (size_t j = 0; j < instruments.size(); ++j)
				{
					if (IDeA::permissive_extract<LT::Str>(*weightedInstrumentTbl,  IDeA_KEY(WEIGHTEDINSTRUMENTS, IDENTITY), i, instrumentTblID))
					{
						instrumentID = instruments[j]->getIdentity();
					}
					else
					{
						instrumentTblID = IDeA::extract<LT::Str>(*weightedInstrumentTbl,  IDeA_KEY(WEIGHTEDINSTRUMENTS, DESCRIPTION), i);
						instrumentID = instruments[j]->getDescription();
					}
					
					if ((!instrumentType.compareCaseless(instruments[j]->getName())) && (!instrumentTblID.compareCaseless(instrumentID)))
					{
						double weight;
						IDeA::permissive_extract<double>(*weightedInstrumentTbl, IDeA_KEY(WEIGHTEDINSTRUMENTS, WEIGHT), i, weight, 1.00);
						m_weightedInstruments[instruments[j]] = weight;
						break;
					}
				}
			}
		}
	}

	bool SingleCurveBestFitKpp::createKnotPoints( const CalibrationInstruments& instruments, const BaseModelPtr baseModel )
	{
		CalibrationInstruments kpInstruments;		
		std::vector<std::pair<double,double> > extraKp;

		// only keep the b instruments for adding to knot points
		if (m_knotPointInstrumentTbl)
		{
			const LT::TablePtr& kpInstrumentTbl = m_knotPointInstrumentTbl->table;		
			LT::Str instrumentTblID, instrumentID;
				
			for (size_t i = 0; i < kpInstrumentTbl->rowsGet()-1; ++i)
			{
				const LT::Str instrumentType = IDeA::extract<LT::Str>(*kpInstrumentTbl,  IDeA_KEY(KNOTPOINTINSTRUMENTS, TYPE), i);
				for (size_t j = 0; j < instruments.size(); ++j)
				{
					if (IDeA::permissive_extract<LT::Str>(*kpInstrumentTbl,  IDeA_KEY(KNOTPOINTINSTRUMENTS, IDENTITY), i, instrumentTblID))
					{
						instrumentID = instruments[j]->getIdentity();
					}
					else
					{
						instrumentTblID = IDeA::extract<LT::Str>(*kpInstrumentTbl,  IDeA_KEY(KNOTPOINTINSTRUMENTS, DESCRIPTION), i);
						instrumentID = instruments[j]->getDescription();
					}

					if ((!instrumentType.compareCaseless(instruments[j]->getName())) && (!instrumentTblID.compareCaseless(instrumentID)))
					{
						kpInstruments.add(instruments[j]);
						break;
					}
				}
			}
		}

		// populate extra knot points (not associated to instruments
		if (m_extraKpTbl)
		{
			const LT::TablePtr& extraKpTbl = m_extraKpTbl->table;				

			for (size_t i = 0; i < extraKpTbl->rowsGet()-1; ++i)
			{
				const LT::Str extraKpTenor = IDeA::extract<LT::Str>(*extraKpTbl,  IDeA_KEY(EXTRAKNOTPOINTS, TENOR), i);
				const double yearFraction = Tenor(extraKpTenor).asYearFraction();
				double initialValue;
				IDeA::permissive_extract<double>(*extraKpTbl, IDeA_KEY(EXTRAKNOTPOINTS, INITIALVALUE), i, initialValue, 100.00);
				extraKp.push_back(make_pair(yearFraction, initialValue));
			}
		}

		// now cast the model to add those kpInstruments knot points and extra knot points
		if (StripperModelPtr model = std::tr1::dynamic_pointer_cast<StripperModel>(baseModel))
			addKnotPoints<StripperModelPtr>(model, kpInstruments, extraKp, m_nestedKpp);
		else if (InflationModelPtr model = std::tr1::dynamic_pointer_cast<InflationModel>(baseModel))
			addKnotPoints<InflationModelPtr>(model, kpInstruments, extraKp, m_nestedKpp);
		else
			LT_THROW_ERROR( "Best fit Kpp is not supported for this model type" );

		return true;
	}

	void SingleCurveBestFitKpp::onLeastSquaresResidualsAdded( LeastSquaresResiduals& lsr ) const
	{
		for (size_t i = 0; i < lsr.sizeOfInstrumentResidual(); ++i)
		{
			WeightedInstrumentsMap::const_iterator iter = m_weightedInstruments.find(lsr.getInstrumentResidualAt(i)->getInstrument());
			if (iter != m_weightedInstruments.end())
				lsr.getInstrumentResidualAt(i)->setWeight(iter->second);
		}
	}
}