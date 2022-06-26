/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_STRUCTURE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_STRUCTURE_H_INCLUDED
#pragma once

#include "BaseCurve.h"
#include "StructureInstrumentUtils.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    /// Structure handles turns, steps and bumps.
    ///
    /// This class provides a common mechanism for models
    /// to support fine tweaking of the behaviour by supporting
    /// the addition of three types of adjustment
    /// Turn - an increase in the overnight rate for one day
    /// Step - a step change in the overnight rate
    /// Bump - a step change in rate followed by opposite step
    ///        at a later date
    /// The effects of these adjustments are treated as seperable
    /// in the sense that the final discount factor D(x) = S(x)M(x)
    /// where S(x) is the return from the structure getDcf
    /// and M(x) is the model dcf part
    /// 
    /// Design notes - implemented in terms of Curve2 rather than deriving from BaseCurve
    /// to ensure initial code only uses getDcf or getLogFvf
    /// Currently does not support fast sensitivity of adjustments
    class Structure
    {
    public:
        explicit Structure(const LTQuant::GenericDataPtr data);
        const double getLogFvf(const double x) const;
        const double getDcf(const double x) const;
    
		LTQuant::GenericDataPtr getCurveDetails() const;

	private:
        BaseCurvePtr m_curve;
		
		// Lightweight structure instrument class
		class Instrument
		{
		public:
			Instrument(const double time,
					   const double spread,
					   const std::string type,
					   const LT::date& date_):
				m_time(time),
				m_spread(spread),
				m_type(type),
				m_maturity(date_.toSimpleString())
			{
			}

			inline double time() const						{ return m_time;		}
			inline double spread() const					{ return m_spread;		}
			inline const std::string& type() const			{ return m_type;		}
			inline const std::string& maturity() const		{ return m_maturity;	}
			//	inline void setType(const std::string& type)	{ m_type = type;		}
			inline void merge(const Instrument& instrument)
			{
				if(LTQuant::doubleEquals(m_time, instrument.time()))
				{
					m_spread += instrument.spread();
					m_type.append(", ").append(instrument.type());
				}
			}

		private:
			double m_time;
			double m_spread;
			std::string m_type;
			std::string m_maturity;
		};	// Instrument

		class Instruments
		{
		private:
			typedef std::vector<Instrument> InstrumentContainer;

		public:
			// Structure instrument comparison functor
			struct Compare
			{			
				inline bool operator()(const Instrument& lhs, const Instrument& rhs) const 
				{
					return operator()(lhs.time(), rhs.time());
				}
				inline bool operator()(const Instrument& lhs, const double rhs) const
				{
					return operator()(lhs.time(), rhs);
				}
				inline bool operator()(const double lhs, const Instrument& rhs) const
				{
					return operator()(lhs, rhs.time());
				}
				inline bool operator()(const double lhs, const double rhs) const
				{
					return (lhs < rhs);
				}
			};

			void add(const Instrument& instrument)
			{
				InstrumentContainer::iterator iter(lower_bound(m_instruments.begin(), m_instruments.end(), instrument.time(), Compare()));
				if(iter != m_instruments.end() && !Compare()(instrument.time(), *iter))
				{
					// an instrument with the same time already exists, merge it with this one
					iter->merge(instrument);
				}
				else
				{
					// no instrument with the same time exists, so add the instrument
					m_instruments.insert(iter, instrument);
				}
			}
			
			inline size_t size() const										{ return m_instruments.size(); }
			inline const Instrument& operator[](const size_t index)	const	{ return m_instruments[index]; }

		private:
			InstrumentContainer m_instruments;
		};	//	Instruments

		Instruments m_instruments;
    };	//	Structure

	

    DECLARE_SMART_PTRS(Structure)
}
#endif //__LIBRARY_PRICERS_FLEXYCF_STRUCTURE_H_INCLUDED