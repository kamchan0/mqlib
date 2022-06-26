/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INFLATIONINDEXARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INFLATIONINDEXARGUMENTS_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"

// STD
#include <functional>

namespace FlexYCF
{
	struct ILIndexArg;

    class InflationIndexArguments
    {
    public:
		typedef std::tr1::function<const ILIndexArg(const LT::date&, const LT::date&)> Creator_Type;

        InflationIndexArguments(const double forward1Time, const double forward2Time, const double weight, const LT::date& payDate)
			: m_forward1Time(forward1Time), m_forward2Time(forward2Time), m_weight(weight), m_payDate(payDate)
		{};

        struct Compare
        {
            bool operator()(const InflationIndexArguments& lhs, const InflationIndexArguments& rhs)
            {
                return (lhs.m_forward1Time < rhs.m_forward1Time || 
                        lhs.m_forward1Time == rhs.m_forward1Time && lhs.m_forward2Time < rhs.m_forward2Time  || 
                        lhs.m_forward1Time == rhs.m_forward1Time && lhs.m_forward2Time == rhs.m_forward2Time && lhs.m_weight < rhs.m_weight );
            }   
        };

        bool operator==(const InflationIndexArguments& other) const
        {
            return m_forward1Time == other.m_forward1Time && 
                   m_forward2Time == other.m_forward2Time &&
                   m_weight == other.m_weight;
        }

		double getForward1Time() const
		{
			return m_forward1Time;
		}

		double getForward2Time() const
		{
			return m_forward2Time;
		}

		double getWeight() const
		{
			return m_weight;
		}

        LT::date getPayDate() const
        {
            return m_payDate;
        }

        virtual std::ostream& print(std::ostream& out) const
        {
            out << "forward 1 Time = " << m_forward1Time << ", forward 2 Time = " << m_forward2Time << ", weight = " << m_weight;
            return out;
        }
    private:
		
		double		m_forward1Time;
		double		m_forward2Time;
		double		m_weight;
       
        LT::date	m_payDate; // not strictly necessary, for DEBUG

    };  

    size_t hash_value(const InflationIndexArguments& inflationIndexArgs);


    namespace
	{
		std::ostream& operator<<(std::ostream& out, const InflationIndexArguments& InflationIndexArguments)
		{
			return InflationIndexArguments.print(out);
		}
	}
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_INFLATIONINDEXARGUMENTS_H_INCLUDED