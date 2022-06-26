/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DISCOUNTFACTORARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DISCOUNTFACTORARGUMENTS_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "Currency.h"

namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
}


namespace FlexYCF
{
    class GlobalComponentCache;

    class DiscountFactorArguments
    {
    public:
        DiscountFactorArguments(const LT::date valueDate, const LT::date payDate, const LTQC::Currency& ccy= "", const LT::Str& index = "");

        double getFlowTime() const
        {
            return m_flowTime;
        }

        struct Compare
        {
            bool operator()(const DiscountFactorArguments& lhs, const DiscountFactorArguments& rhs)
            {
                return (lhs.m_flowTime < rhs.m_flowTime || 
                        lhs.m_flowTime == rhs.m_flowTime && (lhs.m_ccy < rhs.m_ccy  || 
                                                             lhs.m_ccy.compareCaseless(rhs.m_ccy) == 0 && lhs.m_index < rhs.m_index ));
            }   
        };

        bool operator==(const DiscountFactorArguments& other) const
        {
            return m_flowTime == other.m_flowTime && 
                   m_ccy.compareCaseless(other.m_ccy) == 0 &&
                   m_index.compareCaseless(other.m_index) == 0;
        }

        LT::date getPayDate() const
        {
            return m_payDate;
        }

        LTQC::Currency getCurrency() const
        {
            return m_ccy;
        }

        LT::Str getIndex() const
        {
            return m_index;
        }
        virtual std::ostream& print(std::ostream& out) const
        {
            out << "flow time = " << m_flowTime << "Ccy = " << m_ccy.data() << m_index.data();
            return out;
        }
    private:
        double         m_flowTime;
        LTQC::Currency m_ccy;
        LT::Str        m_index;
       
        LT::date m_payDate; // not strictly necessary, for DEBUG

    };  

    size_t hash_value(const DiscountFactorArguments& discountFactorArgs);


    namespace
	{
		std::ostream& operator<<(std::ostream& out, const DiscountFactorArguments& discountFactorArguments)
		{
			return discountFactorArguments.print(out);
		}
	}
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_DISCOUNTFACTORARGUMENTS_H_INCLUDED