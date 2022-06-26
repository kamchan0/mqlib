/*****************************************************************************
    Todo: - Add source file description


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	Standard
#include <algorithm>

//	FlexYCF
#include "StructureCurve.h"
#include "StructureInstrumentCurve.h"
#include "StructureInstrumentCurveUtils.h"
#include "StructureInstrumentCurveFactory.h"
#include "SpineCurvePoint.h"
#include "BaseModel.h"
#include "FlexYcfUtils.h"
#include "RiskInstrument.h"

//	LTQuantLib
#include "Data/GenericData.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"

//	LTQC
#include "QCException.h"

using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
	StructureCurve::StructureCurve(const LTQuant::GenericData& masterTable)
	{
		const GenericDataPtr curveDetailsTable(IDeA::extract<LTQuant::GenericDataPtr>(masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date buildDate(IDeA::extract<LT::date>(*curveDetailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        const GenericDataPtr instrumentListData(IDeA::extract<LTQuant::GenericDataPtr>(masterTable, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST)));

		const size_t nbInstrumentTypes(instrumentListData->numTags());

		StructureInstrumentCurveList tmpCurves;

		//	Load Turn, Bump and Step instrument curves
		for(size_t i(0); i < nbInstrumentTypes; ++i)
		{
			const string instrumentTypeName(instrumentListData->get<string>(i, 0));
            const GenericDataPtr structureInstrumentData(instrumentListData->get<GenericDataPtr>(i, 1));
         
			if(structureInstrumentData)
			{
				//	Fill the temporary curves
				StructureInstrumentCurveFactory::loadStructureInstrumentCurves(instrumentTypeName,
																			   buildDate,
																			   *structureInstrumentData,
																			   tmpCurves);

				//	Insert the new instrument structure curve (of the same structure instrument type)
				//	at the end
				m_instrumentCurves.insert(m_instrumentCurves.end(), tmpCurves.begin(), tmpCurves.end());
			}
		}
	}
	 
	StructureCurve::StructureCurve(const StructureInstrumentCurves& instrumentCurves)
	{
		m_instrumentCurves.insert(m_instrumentCurves.end(), instrumentCurves.begin(), instrumentCurves.end());
	}

	double StructureCurve::getLogFvf(const double time) const
	{
		double logFvfSum(0.0);

		for(StructureInstrumentCurves::const_iterator iter(m_instrumentCurves.begin());
			iter != m_instrumentCurves.end(); ++iter)
		{
			logFvfSum += (*iter)->interpolate(time);
		}

		return logFvfSum;
	}

	/*
	void StructureCurve::computeAnalyticalDelta(InstrumentDeltaVector& instrumentDeltas)
	{
		//	for each struct instrument (curve)
		// accumulate	
		
	}
	*/

	void StructureCurve::accumulateDiscountFactorGradient(const double time,
														  const double multiplier,
														  const GradientIterator begin,
														  const GradientIterator end) const
	{
		if(distance(m_instrumentCurves.begin(), m_instrumentCurves.end()) > distance(begin, end))
		{
			//	TODO: throw error: structure gradient/delta can't fit into the specified [begin, end) range
			LTQC_THROW( LTQC::MathQCException, "The specified range is too small to receive all structure deltas." );
		}
		
		std::transform(m_instrumentCurves.begin(), m_instrumentCurves.end(), begin, ScaledRateDerivativeFunctor(time, multiplier));
	}


	InstrumentDeltaVector StructureCurve::calculateDelta(const BaseModel& model,
													   const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
													   const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows) const
	{
		Gradient gradient(numberOfInstruments(), 0.0);
		
		for(IDeA::ReplicatingFlows<IDeA::Funding>::const_iterator iter(fundingRepFlows.begin()); iter != fundingRepFlows.end(); ++iter)
		{
			accumulateDiscountFactorGradient(iter->getTime(), 
											 -iter->getValue() * model.getDiscountFactor(iter->getTime()),
											 gradient.begin(), 
											 gradient.end());
		}

		for(IDeA::ReplicatingFlows<IDeA::Index>::const_iterator iter(indexRepFlows.begin()); iter != indexRepFlows.end(); ++iter)
		{
			accumulateDiscountFactorGradient(iter->getTime(), 
											 -iter->getValue() * model.getTenorDiscountFactor(iter->getTime(), iter->getTenor()),
											 gradient.begin(), 
											 gradient.end());
		}
		
		InstrumentDeltaVector deltaVector(numberOfInstruments());

		for(size_t i(0); i < numberOfInstruments(); ++i)
		{
			deltaVector[i] = InstrumentDelta(getInstrument(i), 0.0001 * gradient[i], 0.0, DeltaType::TurnDelta);
		}

		return deltaVector;
	}
	/*
	StructureInstrumentList StructureCurve::getInstrumentList() const
	{
		StructureInstrumentList instruments(m_instrumentCurves.size());
		
		std::transform(m_instrumentCurves.begin(), m_instrumentCurves.end(), instruments.begin(), StructureInstrumentCurveToInstrument());
		return instruments;
	}
	*/
	LTQuant::GenericDataPtr StructureCurve::getCurveDetails() const
	{
		//	Create a temporary associative container of spineCurvePoint's containing one point for each step, 
		//	and two points for each turn and bump (for their start and end date) - count them
		SpineCurvePointVector spineCurvePoints;

		for(StructureInstrumentCurves::const_iterator iter(m_instrumentCurves.begin());
			iter != m_instrumentCurves.end(); ++iter)
		{
			(*iter)->addSpineCurvePoints(spineCurvePoints);
		}

		//	Sort x-ascendingly this container 
		std::sort(spineCurvePoints.begin(), spineCurvePoints.end(), SpineCurvePoint::xCompare());

		const GenericDataPtr spineCurveData(new GenericData("Structure Spine Curve", 0));
		
		//	Iterate through each spine curve point
		for(size_t k(0); k < spineCurvePoints.size(); ++k)
		{
			SpineCurvePoint& scPt(spineCurvePoints[k]);

			//	Calculate discount factors (of the whole structure curve)
			scPt.setDiscountFactor(StructureCurve::getDiscountFactor(scPt.x()));
		
			// Dump to generic data
			IDeA::inject<long>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, NUMBER), k, static_cast<long>(k));
			IDeA::inject<std::string>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, INSTRUMENT), k, scPt.description().string());
			IDeA::inject<std::string>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, MATURITY), k, scPt.date().string());
			IDeA::inject<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, X), k, scPt.x());
			IDeA::inject<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, Y), k, scPt.y());
			IDeA::inject<long>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, FIXED), k, scPt.isFixed());
			IDeA::inject<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, DF), k, scPt.discountFactor());

		}

		return spineCurveData;
	}

    void StructureCurve::getUnfixedKnotPoints(std::list<double>& points) const
    {
        SpineCurvePointVector spineCurvePoints;

        for(StructureInstrumentCurves::const_iterator iter(m_instrumentCurves.begin());
            iter != m_instrumentCurves.end(); ++iter)
        {
            (*iter)->addSpineCurvePoints(spineCurvePoints);
        }

        //	Sort x-ascendingly this container 
        std::sort(spineCurvePoints.begin(), spineCurvePoints.end(), SpineCurvePoint::xCompare());

        //	Iterate through each spine curve point
        for(size_t k(0), m=spineCurvePoints.size(); k < m; ++k)
        {
            SpineCurvePoint& scPt(spineCurvePoints[k]);

            if(!scPt.isFixed())
                points.push_back(scPt.x());
        }
    }

	StructureSurface::StructureSurface(const LTQuant::GenericData& masterTable) : m_defaultCurve(new StructureCurve)
	{
		const GenericDataPtr curveDetailsTable(IDeA::extract<LTQuant::GenericDataPtr>(masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date buildDate(IDeA::extract<LT::date>(*curveDetailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        const GenericDataPtr instrumentListData(IDeA::extract<LTQuant::GenericDataPtr>(masterTable, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST)));
		GenericDataPtr turnsInstrumentData, stepsInstrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(*instrumentListData, IDeA_KEY(YC_INSTRUMENTLIST,TURNS), turnsInstrumentData, turnsInstrumentData);
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(*instrumentListData, IDeA_KEY(YC_INSTRUMENTLIST,STEPS), stepsInstrumentData, stepsInstrumentData);

		// multi-turns case
		if(turnsInstrumentData || stepsInstrumentData)
		{
			std::map<CurveTypeConstPtr, StructureCurve::StructureInstrumentCurves, CurveType::DereferenceLess> instrumentCurvesMap;

			if(stepsInstrumentData)
			{
				LT::TablePtr table = stepsInstrumentData->table;
				size_t rows = table->rowsGet();
				size_t k1 = table->findColKey(LT::NoThrow,IDeA_KEY(STEPSBYTENOR, TENOR).getName());
				size_t k2 = table->findColKey(LT::NoThrow,IDeA_KEY(STEPSBYTENOR, STEPS).getName());
				if(k1 != LT::Table::not_found && k2 != LT::Table::not_found)
				{
					for(size_t j = 1; j < rows; ++j)
					{	
						LT::TablePtr stepsTable = table->at(j,k2);
						LT::Str tenorStr = table->at(j,k1);
						std::string curveDescription(tenorStr.cStr());
						CurveTypeConstPtr curveType = CurveType::getFromDescription(curveDescription);
						LTQuant::GenericDataPtr instrumentTable(new GenericData(stepsTable));
			
						StructureInstrumentCurveList tmpCurves;
						StructureInstrumentCurveFactory::loadStructureInstrumentCurves("STEPS", buildDate, *instrumentTable, tmpCurves);
						instrumentCurvesMap[curveType].insert(instrumentCurvesMap[curveType].end(), tmpCurves.begin(), tmpCurves.end());
					}
				}
			}

			if(turnsInstrumentData)
			{
				LT::TablePtr table = turnsInstrumentData->table;
				size_t rows = table->rowsGet();
				size_t k1 = table->findColKey(LT::NoThrow,IDeA_KEY(TURNSBYTENOR, TENOR).getName());
				size_t k2 = table->findColKey(LT::NoThrow,IDeA_KEY(TURNSBYTENOR, TURNS).getName());
				if(k1 != LT::Table::not_found && k2 != LT::Table::not_found)
				{
					for(size_t j = 1; j < rows; ++j)
					{	
						LT::TablePtr turnsTable = table->at(j,k2);
						LT::Str tenorStr = table->at(j,k1);
						std::string curveDescription(tenorStr.cStr());
						CurveTypeConstPtr curveType = CurveType::getFromDescription(curveDescription);
						LTQuant::GenericDataPtr instrumentTable(new GenericData(turnsTable));
			
						StructureInstrumentCurveList tmpCurves;
						StructureInstrumentCurveFactory::loadStructureInstrumentCurves("TURNS", buildDate, *instrumentTable, tmpCurves);
						instrumentCurvesMap[curveType].insert(instrumentCurvesMap[curveType].end(), tmpCurves.begin(), tmpCurves.end());
					}
				}
				
			}

			for(auto it = instrumentCurvesMap.begin(); it!=instrumentCurvesMap.end(); ++it)
			{
				StructureCurvePtr structureCurve(new StructureCurve(it->second));
				m_curveTypes.push_back(it->first);
				m_curves.push_back(structureCurve);
			}
			if(m_curves.size() > 0) return;
		}

		// single curve
		const size_t nbInstrumentTypes(instrumentListData->numTags());
		StructureInstrumentCurveList tmpCurves;
		StructureCurve::StructureInstrumentCurves instrumentCurves;
		//	Load Turn, Bump and Step instrument curves
		for(size_t i(0); i < nbInstrumentTypes; ++i)
		{
			const string instrumentTypeName(instrumentListData->get<string>(i, 0));
            const GenericDataPtr structureInstrumentData(instrumentListData->get<GenericDataPtr>(i, 1));
         
			if(structureInstrumentData)
			{
				StructureInstrumentCurveFactory::loadStructureInstrumentCurves(instrumentTypeName, buildDate, *structureInstrumentData, tmpCurves);
				instrumentCurves.insert(instrumentCurves.end(), tmpCurves.begin(), tmpCurves.end());
			}
		}

		if(instrumentCurves.size() > 0)
		{
			CurveTypeConstPtr curveType = CurveType::getFromDescription(std::string("3M"));
			StructureCurvePtr structureCurve(new StructureCurve(instrumentCurves));
			m_curveTypes.push_back(curveType);
			m_curves.push_back(structureCurve);
		}
	}
}
