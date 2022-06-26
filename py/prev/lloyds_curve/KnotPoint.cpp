/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "KnotPoint.h"
#include "FlexYCFCloneLookup.h"
#include "CalibrationInstrument.h"

using namespace LTQC;

namespace FlexYCF
{
	KnotPoint::KnotPoint(const double x_, const double y_, const bool isKnown_):
		x(x_),
        y(y_),
        isKnown(isKnown_)
    {
	}

    KnotPoint::KnotPoint(const double x_, const double y_, const bool isKnown_, const CalibrationInstrumentPtr& relatedInstrument):
        x(x_),
        y(y_),
        isKnown(isKnown_),
		m_relatedInstrument(relatedInstrument)
    {
	}

    /**
        @brief Pseudo copy constructor.

        Use a lookup to maintain directed graph relationships.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    KnotPoint::KnotPoint(KnotPoint const& original, CloneLookup& lookup) :
        x(original.x),
        y(original.y),
        isKnown(original.isKnown),
        m_relatedInstrument(lookup.get(original.m_relatedInstrument))
    {
    }

	const KnotPoint& KnotPoint::origin()
	{
		static const KnotPoint the_origin(0.0, 0.0, true);
		return the_origin;
	}

	const KnotPoint& KnotPoint::xNegativeHelper()
	{
		static const KnotPoint the_x_negative_helper(-1.0, 0.0, true);
		return the_x_negative_helper;
	}

    std::ostream& KnotPoint::print(std::ostream& out) const
    {
        out << "(" << x << ", " << y << (isKnown?")":"?)");
        return out;
    }

}   // FlexYCF