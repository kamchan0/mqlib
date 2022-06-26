/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InterpolationCurve.h"
#include "FlexYCFCloneLookup.h"

using namespace std;

namespace FlexYCF
{
	
	void InterpolationCurve::addUnknownsToProblem(const LTQuant::ProblemPtr& problem,
												  IKnotPointFunctor const& onKnotPointVariableAddedToProblem)
	{
		for(size_t kpIndex(0); kpIndex < m_knotPoints->size(); ++kpIndex)
		{
			addKnotPointVariableToProblem(kpIndex, problem);
			// maybe a test to invoke the call back only if a variable related 
			// to a knot-point has been added
			onKnotPointVariableAddedToProblem(getKnotPoint(kpIndex));
		}
	}

    void InterpolationCurve::addKnotPoint(const KnotPoint& knotPoint)
    {
        // We chose to enforce this design at this level 
        // and not at ICurve or BaseCurve level.
        try 
        {
            m_knotPoints->add(knotPoint);
            onKnotPointAdded(knotPoint);
        }
        catch(exception& exc) 
        {
            LT_INFO << exc.what();
        }
    }

    // Implementation note: needs to be public because
    //  Composite curves need to delegate
    void InterpolationCurve::onKnotPointAdded(const KnotPoint& /* knotPoint */)
    {
        // do nothing
    }

    /* Not used yet
    void initializeKnotPoints(const KnotPoints::InitFunction& func);
    virtual void onKnotPointsInitialized()
    {
    }
        
    void InterpolationCurve::initializeKnotPoints(const KnotPoints::InitFunction& func)
    {
        setYsFromXsWithFunction(func);
        onKnotPointsInitialized();
    }
    */

    bool InterpolationCurve::empty() const
    {
        return m_knotPoints->empty();
    }

    double InterpolationCurve::xMin() const
    {
        return m_knotPoints->xMin();
    }

    double InterpolationCurve::xMax() const
    {
        return m_knotPoints->xMax();
    }

    size_t InterpolationCurve::size() const
    {
        return m_knotPoints->size();
    }

    const KnotPoint& InterpolationCurve::getKnotPoint(const size_t index) const
    {
        if(index >= m_knotPoints->size())
        {
            LT_THROW_ERROR("index is out of range in InterpolationCurve::getKnotPoint.");
        }
        return m_knotPoints->operator[](index);
    }
    
    KnotPoint& InterpolationCurve::getKnotPoint(const size_t index)
    {
        if(index >= m_knotPoints->size())
        {
            LT_THROW_ERROR("index is out of range in InterpolationCurve::getKnotPoint.");
        }
        return m_knotPoints->operator[](index);
    }

    InterpolationCurve::InterpolationCurve()
    {
    }

    /**
        @brief Pseudo copy constructor.

        Create a copy using a lookup to preserved directed graph relationships.

        @param original The original instance to copy.
        @param lookup   The lookup of previously created clones.
    */
    InterpolationCurve::InterpolationCurve(InterpolationCurve const& original, CloneLookup& lookup) :
        m_knotPoints(lookup.get(original.m_knotPoints))
    {
    }

    void InterpolationCurve::setKnotPoints(const KnotPointsPtr knotPoints)
    {
        m_knotPoints = knotPoints;
    }
    
    void InterpolationCurve::addUnknownKnotPointsYsToProblemInRange(const LTQuant::ProblemPtr& problem,
                                                                    const double min,
                                                                    const double max)
    {
        m_knotPoints->addUnknownKnotPointsYsToProblemInRange(problem, min, max);
    }

    void InterpolationCurve::setYsFromXsWithFunction(const KnotPoints::InitFunction& func)
    {
        m_knotPoints->setYsFromXsWithFunction(func);
    }

    KnotPoints::const_iterator InterpolationCurve::begin() const
    {
        return m_knotPoints->begin();
    }

    KnotPoints::const_iterator InterpolationCurve::end() const
    {
        return m_knotPoints->end();
    }

    KnotPoints::const_iterator InterpolationCurve::lowerBound(const double x) const
    {
        return m_knotPoints->lowerBound(x);
    }

    KnotPoints::const_iterator InterpolationCurve::upperBound(const double x) const
    {
        return m_knotPoints->upperBound(x);
    }

    /// Returns the number of unknown knot-points
    size_t InterpolationCurve::getNumberOfUnknownKnotPoints() const
    {
        return m_knotPoints->getNumberOfUnknowns();
    }
   

}