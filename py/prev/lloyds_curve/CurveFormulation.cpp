/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "CurveFormulation.h"
#include "ICurveFactory.h"
#include "Data\GenericData.h"
#include "BaseCurve.h"
#include "KnotPoint.h"
#include "KnotPoints.h"
#include "FlexYCFCloneLookup.h"
#include "DataExtraction.h"
#include "DictYieldCurve.h"

using namespace std;
using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
    CurveFormulation::CurveFormulation(const GenericDataPtr& interpolationDetailsTable,
                                       const string& curveDescription,
                                       const LeastSquaresResidualsPtr& leastSquaresResiduals):
        m_curveDescription(curveDescription)
    {
        const string defaultCurveTypeName(BaseCurve::getName());  
        
		// Spot rate initialization should not be done here
		//	-->	to be encapsulated in a separate class
		const double defaultInitSpotRate(0.05);     // this initialization to an equivalent spot rate might 
                                                    
        string curveTypeName(defaultCurveTypeName);
        m_initSpotRate = defaultInitSpotRate;

        LTQuant::GenericDataPtr singleCurveInterpolationTable;
        
        if(static_cast<bool>(interpolationDetailsTable))
        {
            interpolationDetailsTable->permissive_get<LTQuant::GenericDataPtr>(curveDescription,
                                                                               0,
                                                                               singleCurveInterpolationTable,
                                                                               LTQuant::GenericDataPtr());
            if(static_cast<bool>(singleCurveInterpolationTable))
            {
                singleCurveInterpolationTable->permissive_get<string>("Curve Type", 0, curveTypeName, defaultCurveTypeName);
                singleCurveInterpolationTable->permissive_get<double>("Init Rate", 0, m_initSpotRate, defaultInitSpotRate);
            }
        }

		m_baseCurve = std::tr1::dynamic_pointer_cast<BaseCurve>(
            ICurveFactory::createInstance(curveTypeName, singleCurveInterpolationTable, leastSquaresResiduals)
            );
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to preserve the directed graph relationships of the original instance.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    CurveFormulation::CurveFormulation(CurveFormulation const& original, CloneLookup& lookup) : 
        m_baseCurve(lookup.get(original.m_baseCurve)),
        m_curveDescription(original.m_curveDescription),
        m_initSpotRate(original.m_initSpotRate)
    {
    }

    void CurveFormulation::addKnotPoint(const KnotPoint& knotPoint) const
    {
        m_baseCurve->addKnotPoint(knotPoint);
    }

    size_t CurveFormulation::getNumberOfUnknowns() const
    {
        return m_baseCurve->getNumberOfUnknowns();
    }

    void CurveFormulation::addKnotPoint(const double x, const double y) const
    {
        m_baseCurve->addKnotPoint(KnotPoint(x, y, false));
    }

    void CurveFormulation::addFixedKnotPoint(const double x, const double y) const
    {
        m_baseCurve->addKnotPoint(KnotPoint(x, y, true));
    }

    void CurveFormulation::setSpotRate(double spotTime, double spotRate)
    {
    }

    void CurveFormulation::addUnknownsToProblem(const LTQuant::ProblemPtr problem) const
    {
        m_baseCurve->addUnknownsToProblem(problem);
    }

	void CurveFormulation::addUnknownsToProblem(const LTQuant::ProblemPtr problem,
												IKnotPointFunctor& onKnotPointVariableAddedToProblem) const
	{
		m_baseCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
	}
    
	void CurveFormulation::updateVariablesFromShifts(const LTQC::VectorDouble& variableShifts)
	{
		for(size_t k(0); k < variableShifts.size(); ++k)
		{
			m_baseCurve->shiftUnknown(k, variableShifts[k]);
		}
	}
    
	void CurveFormulation::finalize() const
    {
        onFinalize();    // to allow some formulations to add a required fixed knot at time 0 if missing
        m_baseCurve->finalize();
	}

    void CurveFormulation::update() const
    {
        m_baseCurve->update();
    }

	void CurveFormulation::onKnotPointsInitialized() const
	{
		m_baseCurve->onKnotPointsInitialized();
	}

    double CurveFormulation::getInitSpotRate() const
    {
        return m_initSpotRate;
    }
	KnotPoints::const_iterator CurveFormulation::begin() const
	{
		return m_baseCurve->begin();
	}
	KnotPoints::const_iterator CurveFormulation::end() const
	{
		return m_baseCurve->end();
	}

	LTQuant::GenericDataPtr CurveFormulation::getSpineCurveDetails() const
	{
		LTQuant::GenericDataPtr spineCurveData(m_baseCurve->getCurveDetails());

		for(size_t k(0); k < spineCurveData->numItems() - 1; ++k)
		{
			double df = getDiscountFactor(spineCurveData->get<double>("x", k));
			IDeA::inject<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, DF), k, df);
			// spineCurveData->set<double>("df", k, getDiscountFactor(spineCurveData->get<double>("x", k)));
		}

		return spineCurveData;
	}

	void CurveFormulation::getCurveInternalData(knot_points_container& kpc) const {
		m_baseCurve->getCurveInternalData(kpc);
	}

	void CurveFormulation::assignCurveInternalData(knot_points_container::const_iterator it) {
		m_baseCurve->assignCurveInternalData(it);
	}

    void CurveFormulation::getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const
    {
        m_baseCurve->getUnfixedKnotPoints(points);
    }
	
	std::vector<double> CurveFormulation::abscissas() const
	{
		std::vector<double> x;
		KnotPoints::const_iterator it = m_baseCurve->begin();
		for( ;it != m_baseCurve->end(); ++it)
		{
			x.push_back(it->x);
		}
		return x;
	}

    void CurveFormulation::onFinalize() const
    {
        // Do nothing by default
    }

    void CurveFormulation::enforceFixedKnotPoint(const double x, const double y) const
    {
        // same as InterpolationMethod::lowerBound
        KnotPoints::const_iterator lower(lower_bound(m_baseCurve->begin(), m_baseCurve->end(), x, KnotPoint::xCompare()));
        
        if(lower == m_baseCurve->end() || lower->x != x)
        {
            // If there is no (fixed) knot-point at x, add (x, y)!
            m_baseCurve->addKnotPoint(KnotPoint(x, y, true));
        }
        else if((lower->y != y) || !lower->isKnown)
        {
            LT_THROW_ERROR("the knot-point at time " << x << " should have value " << y << " and be fixed");
        }
    }
}