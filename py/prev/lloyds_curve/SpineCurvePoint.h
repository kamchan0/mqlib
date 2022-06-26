/*****************************************************************************
    SpineCurvePoint

	Represents a point in a spine curve. A spine curve point is a knot-point
	with financial information 


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __SpineCurvePoint_H__
#define __SpineCurvePoint_H__


//	LTQuantLib
#include "lt/const_string.h"
#include "ModuleDate/InternalInterface/Utils.h"

//	FlexYCF
#include "KnotPoint.h"
#include "FlexYcfUtils.h"


namespace FlexYCF
{
	class SpineCurvePoint: private KnotPoint
	{
	public:
		SpineCurvePoint(const double x, 
						const double y,
						const bool isFixed,
						const LT::Str& description,
						const LT::date& date_,
						const double discountFactor	= 0.0):
			KnotPoint(x, y, isFixed),
			m_description(description),
			m_date(LT::Str(date_.toSimpleString())),
			m_discountFactor(discountFactor)
		{
		}

		//	Returns the x-coordinate of the knot-point (x,y)
		inline double x() const		
		{ 
			return KnotPoint::x;			
		}

		//	Returns the y-coordinate of the knot-point (x,y)
		inline double y() const		
		{ 
			return KnotPoint::y;			
		}

		//	Indicates whether the point is fixed (i.e. known)
		//	or not
		inline bool isFixed() const	
		{
			return KnotPoint::isKnown;	
		}

		//	Return the description
		inline const LT::Str& description() const
		{
			return m_description;
		}

		//	Return a date, in string format, such that
		//	the number of years between the build date
		//	and this one is the x-coordinate of the point
		inline const LT::Str& date() const
		{
			return m_date;
		}

		//	Returns the discount factor
		inline double discountFactor() const
		{
			return m_discountFactor;
		}

		//	Sets the discount factor
		inline void setDiscountFactor(const double discountFactor)
		{
			m_discountFactor = discountFactor;
		}
		
		//	Comparison functor
		struct xCompare
		{
			bool operator()(const SpineCurvePoint& lhs, const SpineCurvePoint& rhs) const
			{
				return (lhs.x() < rhs.x());
			}
		};

	private:
		LT::Str		m_description;
		LT::Str		m_date;
		double		m_discountFactor;
	};


	DEFINE_VECTOR( SpineCurvePoint );
}

#endif	//	__SpineCurvePoint_H__
