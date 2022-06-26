/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TENORDISCOUNTFACTORARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TENORDISCOUNTFACTORARGUMENTS_H_INCLUDED
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

    class TenorDiscountFactorArguments
    {
    public:
        TenorDiscountFactorArguments() { }

        TenorDiscountFactorArguments(const LT::date valueDate,
                                     const LT::date payDate,
                                     const double tenor,
                                     const LTQC::Currency& ccy = "",
                                     const LT::Str& index = "");

       TenorDiscountFactorArguments(const LT::date valueDate,
                                     const LT::date payDate,
                                     const std::string& tenorDescription,
                                     const LTQC::Currency& ccy = "",
                                     const LT::Str& index = "");

        /// Returns the number of years between the 
        /// value date and the pay date
        double getFlowTime() const
        {
            return m_flowTime;
        }
        
        /// Returns the Tenor
        double getTenor() const
        {
            return m_tenor;
        }

        LTQC::Currency getCurrency() const
        {
           return  m_ccy; 
        }
        
        LT::Str getIndex() const
        {
           return  m_index; 
        }
        
        LT::date getPayDate() const
        {
            return m_payDate;
        }

        struct Compare
        {
            bool operator()(const TenorDiscountFactorArguments& lhs, const TenorDiscountFactorArguments& rhs)
            {
                return (lhs.m_flowTime < rhs.m_flowTime || 
                        lhs.m_flowTime == rhs.m_flowTime && (lhs.m_tenor < rhs.m_tenor || 
                                                             lhs.m_tenor == rhs.m_tenor && (lhs.m_ccy < rhs.m_ccy  || 
                                                                                            lhs.m_ccy.compareCaseless(rhs.m_ccy) == 0 &&  lhs.m_index < rhs.m_index )));
            }

        };

        bool operator==(const TenorDiscountFactorArguments& other) const
        {
            return m_flowTime == other.m_flowTime && 
                   m_tenor == other.m_tenor && 
                   m_ccy.compareCaseless(other.m_ccy) == 0 &&
                   m_index.compareCaseless(other.m_index) == 0;
        }

        virtual std::ostream& print(std::ostream& out) const
        {
            out << "Tenor DF " << m_ccy.data() << m_index.cStr();
            return out;
        }

    private:
        double         m_flowTime;
        double         m_tenor;
        LTQC::Currency m_ccy;
        LT::Str        m_index;
        
        LT::date m_payDate; // not strictly necesasry, for DEBUG
    }; 

	size_t hash_value(const TenorDiscountFactorArguments& tenorDiscountFactorArgs);

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const TenorDiscountFactorArguments& tenorDiscountFactorArguments)
		{
			return tenorDiscountFactorArguments.print(out);
		}
	}

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_TENORDISCOUNTFACTORARGUMENTS_H_INCLUDED