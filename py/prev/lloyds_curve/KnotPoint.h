/*****************************************************************************

    KnotPoint

	Represents a knot-point (x,y) on a curve, whether this knot-point
	is known or not.
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_KNOTPOINT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_KNOTPOINT_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( CalibrationInstrument )

    // Forward declarations
    class CloneLookup;

    /// KnotPoint represents a point (x,y) on curve with the additional
    /// boolean isKnown indicating whether it must be considered fixed
    /// or as a variable to that must be determined during calibration.
    ///
    /// Interpolation on a curve depends on the set of its knot-points.
    /// The 'isKnown' member is to distinguish the fixed knot-points of
    /// the curve from those where at least one coordinate is unknown. 
    /// Typically the y-coordinate (value) is unknown while x-coordinate 
    /// (time) is.
    /// As a knot-point on a curve usually represents the information
    /// from a certain instrument market quote (possibly several),
    /// it is possible to add instruments to the knot-point and to get 
    /// the formed collection of instruments.
    class KnotPoint
    {
    public:
		//typedef std::unary_function<void, KnotPoint&>	Functor;

		KnotPoint(const double x_, const double y_, const bool isKnown_);
        KnotPoint(const double x_, const double y_, const bool isKnown_, const CalibrationInstrumentPtr& relatedInstrument);
        KnotPoint(KnotPoint const& original, CloneLookup& lookup);

        double x;
        double y;
        bool isKnown;

        /// Returns the calibration instrument used to place the knot-point
        inline
        CalibrationInstrumentPtr getRelatedInstrument() const
        {
            return m_relatedInstrument;
        }

		//	Returns the fixed (known), (0,0) knot-point with
		//	no related instrument attached
		static const KnotPoint& origin();

		//	Returns an x-negative fixed knot-point whose only raison
		//	d'etre is to help build the curve
		static const KnotPoint& xNegativeHelper();

        /// A nested functor used to sort knot-points
        struct xCompare 
         {
        public:
            bool operator()(const KnotPoint& lhs, const KnotPoint& rhs) const
            {
                return operator()(lhs.x, rhs.x);
            }

            bool operator()(const KnotPoint& lhs, const double rhs) const
            {
                return operator()(lhs.x, rhs);
            }

            bool operator()(const double lhs, const KnotPoint& rhs) const
            {
                return operator()(lhs, rhs.x);
            }

        private:
            bool operator()(const double lhs, const double rhs) const
            {
                return lhs < rhs;
            }
        };  // xCompare
    
        /// Prints the coordinates of the knot-point
        /// and whether it is known or not
        std::ostream& print(std::ostream& out) const;

    private:
		CalibrationInstrumentPtr	m_relatedInstrument;
	};  // KnotPoint

	namespace 
	{
		/// Lexigraphical order operator for knot-point
		inline
		const bool operator<(const KnotPoint& lhs, const KnotPoint& rhs)
		{
			return ((lhs.x < rhs.x) || ((lhs.x == rhs.x) && (lhs.y < rhs.y)));
		}
	    
		/// Returns whether a knot-point is known or not.
		/// Note: used in STL algorithms such as count_if.
		inline
		const bool isUnknownKnotPoint(const KnotPoint& knotPoint)
		{
			return !(knotPoint.isKnown);
		}

		std::ostream& operator<<(std::ostream& out, const KnotPoint& knotPoint)
		{
			return knotPoint.print(out);
		}
	}
} //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_KNOTPOINT_H_INCLUDED