/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "KnotPoints.h"
#include "Maths\Problem.h"
#include "SolverVariable.h"


namespace FlexYCF
{

    void KnotPoints::add(const KnotPoint& knotPoint)
    {
        KnotPointContainer::iterator lower(lower_bound(m_knotPoints.begin(), m_knotPoints.end(), knotPoint, KnotPoint::xCompare()));
        
        if(lower == m_knotPoints.end() || KnotPoint::xCompare()(knotPoint, *lower))
        {   // insert knot-Point at lower position
            m_knotPoints.insert(lower, knotPoint);
        }
        else
        {   // a knot-point with the same x is already in the knot-points of the curve
            LT_THROW_ERROR( "Error in 'FlexYCF::BaseCurve'. The function 'addKnotPoint' failed because a knot-point with x-coordinate " 
				<< knotPoint.x << " already exists in the curve." << std::endl );
        }
    }
   
    void KnotPoints::addUnknownKnotPointsYsToProblemInRange(const LTQuant::ProblemPtr& problem,
                                                            const double min,
                                                            const double max)
    {
        for(KnotPointContainer::iterator iter(m_knotPoints.begin());
            iter != m_knotPoints.end(); ++iter)
        {
            if(!iter->isKnown && !(KnotPoint::xCompare()(*iter, min)) && KnotPoint::xCompare()(*iter, max))
            {
                problem->addVariable(SolverVariablePtr(new SolverVariable(iter->y)));
            }
        }
    }

    void KnotPoints::setYsFromXsWithFunction(const /*InitFunction*/CurveInitializationFunction& func)
    {
        for(KnotPointContainer::iterator iter(m_knotPoints.begin()); iter != m_knotPoints.end(); ++iter)
        {
            iter->y = func(iter->x);
        }
    }
    
    void KnotPoints::setUnknownYsFromXsWithFunction(const CurveInitializationFunction& func)
    {
        for(KnotPointContainer::iterator iter(m_knotPoints.begin()); iter != m_knotPoints.end(); ++iter)
        {
           if( !iter->isKnown )
            {
                iter->y = func(iter->x);
            }
        }
    }
    bool KnotPoints::empty() const
    {
        return (m_knotPoints.size() == 0);
    }

    double KnotPoints::xMin() const
    {
        checkNonEmpty();
        return m_knotPoints.begin()->x;
    }
  
    double KnotPoints::xMax() const
    {
        checkNonEmpty();
        const_iterator maxIter(m_knotPoints.end() - 1);
        return maxIter->x;
    }

    size_t KnotPoints::getNumberOfUnknowns() const
    {
        return count_if(m_knotPoints.begin(), m_knotPoints.end(), isUnknownKnotPoint);
    }

    /**
        @brief Clone this instance.

        Use a lookup to preserve directed graph relationships.

        @param lookup The lookup of previously created clones.
        @return       A clone of this instance.
    */
    ICloneLookupPtr KnotPoints::cloneWithLookup(CloneLookup& lookup) const
    {
        KnotPointsPtr clone(new KnotPoints);
        clone->m_knotPoints.reserve(m_knotPoints.size());
        for (KnotPointContainer::const_iterator it = m_knotPoints.begin(); it != m_knotPoints.end(); ++it)
            clone->m_knotPoints.push_back(KnotPoint(*it, lookup));
        return clone;
    }

    /*
    std::pair<KnotPointsPtr, KnotPointsPtr> KnotPoints::split(const double x) const
    {
        const_iterator upper(upperBound(x));
    
        // special cases
    }*/
    
    void KnotPoints::checkNonEmpty() const
    {
        if(m_knotPoints.empty())
        {
            LT_THROW_ERROR("m_knotPoints is empty.");
        }
    }


    void upperToLowerBound(const double x,
                           const KnotPointConstIterator begin,
                           KnotPointConstIterator& upperBound)
    {
        if(upperBound != begin)
        {
            --upperBound;
        }
        if(x < upperBound->x)
        {
            LT_THROW_ERROR("Something went wrong with knot-point iterator.");
        }
        if(x > upperBound->x) 
        {
            ++upperBound;
        }
    }
}