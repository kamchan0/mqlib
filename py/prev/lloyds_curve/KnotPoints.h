/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_KNOTPOINTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_KNOTPOINTS_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "KnotPoint.h"
#include "CurveInitializationFunction.h"
#include "ICloneLookup.h"

#include <algorithm>

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( Problem )
}

namespace FlexYCF
{
    /// Represents a collection of ordered (ascendinly in their x-coordinates)
    /// knot-points.
    /// Note: for any double, at most one knot-point with this double as its 
    /// x-coordinate exists in the collection.
    class KnotPoints : public ICloneLookup
    {
        typedef std::vector<KnotPoint>   KnotPointContainer;
    
    public:
        typedef KnotPointContainer::const_iterator const_iterator;
		typedef std::tr1::function<double (const double)> InitFunction;
		// Replace with CurveInitializationFunction (same type)?

        const_iterator begin() const
        {
            return m_knotPoints.begin();
        }

        const_iterator end() const
        {
            return m_knotPoints.end();
        }

        size_t size() const
        {
            return m_knotPoints.size();
        }

        const KnotPoint& operator[](const size_t index) const
        {
            return m_knotPoints[index];
        }

        KnotPoint& operator[](const size_t index)
        {
            return m_knotPoints[index];
        }

        const_iterator lowerBound(const double x) const
        {
            return lower_bound(m_knotPoints.begin(), m_knotPoints.end(), x, KnotPoint::xCompare());
        }

        const_iterator lowerBound(const KnotPoint& knotPoint) const
        {
            return lowerBound(knotPoint.x);
        }

        const_iterator upperBound(const double x) const
        {
            return upper_bound(m_knotPoints.begin(), m_knotPoints.end(), x, KnotPoint::xCompare()); 
        }

        const_iterator upperBound(const KnotPoint& knotPoint) const
        {
            return upperBound(knotPoint.x);
        }

        /// Add a knot-point which x-coordinate is not already in the collection
        /// at the right place.
        /// An error will be thrown otherwise.
        void add(const KnotPoint& knotPoint);
        
        /// Add unknown knot-points whose x is in [min, max) range
        void addUnknownKnotPointsYsToProblemInRange(const LTQuant::ProblemPtr& problem,
                                                    const double min,
                                                    const double max);

        /// This can be used for initialization of the y's from the x's and the TensionSpline::update
        void setYsFromXsWithFunction(const /*InitFunction*/CurveInitializationFunction& func);
        void setUnknownYsFromXsWithFunction(const CurveInitializationFunction& func);
        
        bool empty() const;
        double xMin() const;
        double xMax() const;
        
        /// Returns the number of unknown knot-points.
        size_t getNumberOfUnknowns() const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    private:
        void checkNonEmpty() const;

        KnotPointContainer m_knotPoints;
    };

    DECLARE_SMART_PTRS( KnotPoints )

    typedef KnotPoints::const_iterator KnotPointConstIterator;

    /// Transforms the upper bound to lower bound
    void upperToLowerBound(const double x,
                           const KnotPointConstIterator begin, 
                           KnotPointConstIterator& upperBound);


}

#endif //__LIBRARY_PRICERS_FLEXYCF_KNOTPOINTS_H_INCLUDED 