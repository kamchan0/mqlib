/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TYPEDCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TYPEDCURVE_H_INCLUDED
#pragma once

#include "BaseCurve.h"
#include "CurveType.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( Problem )
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( SolverVariable )
    FWD_DECLARE_SMART_PTRS( TypedCurve )

    /// TypedCurve represents a curve with an attached type.
    class TypedCurve : public BaseCurve
    {
    public:    
        explicit TypedCurve(const CurveTypeConstPtr curveType,
                            const bool addZeroFixedKnotPoint = true);
        
        CurveTypeConstPtr getCurveType() const;

        std::ostream& print(std::ostream& out) const;

        // A nested functor to compare tenor curves and time fractions between them.
        //  Relies on operator<(CurveTypeConstPtr, CurveTypeConstPtr) defined in 'CurveType.h'
        //  Useful in STL algorithms to deal with containers of TypedCurve's.
        struct Compare
        {
        public:
            bool operator()(const TypedCurve& lhs, const TypedCurve& rhs) const
            {
                return compare(lhs.getCurveType(), rhs.getCurveType());
            }

            bool operator()(const TypedCurve& lhs, const double rhs) const
            {
                return compare(lhs.getCurveType(), CurveType::getFromYearFraction(rhs));
            }

            bool operator()(const double lhs, const TypedCurve& rhs) const
            {
                return compare(CurveType::getFromYearFraction(lhs), rhs.getCurveType());
            }

            bool operator()(const CurveTypeConstPtr lhs, const double rhs) const
            {
                return compare(lhs, CurveType::getFromYearFraction(rhs));
            }

            bool operator()(const double lhs, const CurveTypeConstPtr rhs) const
            {
                return compare(CurveType::getFromYearFraction(lhs), rhs);
            }

            bool operator()(const CurveTypeConstPtr lhs, const TypedCurve& rhs) const
            {
                return compare(lhs, rhs.getCurveType());
            }

            bool operator()(const TypedCurve& lhs, const CurveTypeConstPtr rhs) const
            {
                return compare(lhs.getCurveType(), rhs);
            }
            
            bool operator()(const TypedCurvePtr lhs, const TypedCurvePtr rhs) const
            {
                return compare(lhs->getCurveType(), rhs->getCurveType());
            }

            bool operator()(const TypedCurvePtr lhs, const CurveTypeConstPtr rhs) const
            {
                return compare(lhs->getCurveType(), rhs);
            }

            bool operator()(const CurveTypeConstPtr lhs, const TypedCurvePtr rhs) const
            {
                return compare(lhs, rhs->getCurveType());
            }

            bool operator()(const TypedCurvePtr lhs, const double rhs) const
            {
                return compare(lhs->getCurveType(), CurveType::getFromYearFraction(rhs));
            }

            bool operator()(const double lhs, const TypedCurvePtr rhs) const
            {
                return compare(CurveType::getFromYearFraction(lhs), rhs->getCurveType());
            }

            bool operator()(const CurveTypeConstPtr lhs, const CurveTypeConstPtr rhs) const
            {
                return compare(lhs, rhs);
            }

        private:
            bool compare(const CurveTypeConstPtr lhs, const CurveTypeConstPtr rhs) const
            {
                return (lhs < rhs);
            }
        };  // Compare
    private:
        CurveTypeConstPtr m_curveTypePtr;
    };  //  TypedCurve

    DECLARE_SMART_PTRS( TypedCurve )

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_TYPEDCURVE_H_INCLUDED