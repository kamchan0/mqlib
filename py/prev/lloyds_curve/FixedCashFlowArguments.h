/*****************************************************************************

    FixedCashFlowArguments.

	Encapsulates the arguments required to create an instance of 
	FixedCashFlow.
    
	
	@Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FIXEDCASHFLOWARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FIXEDCASHFLOWARGUMENTS_H_INCLUDED
#pragma once

//	FlexYCF
#include "DiscountFactor.h"
#include "DiscountFactorArguments.h"


namespace ModuleDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
}


namespace FlexYCF
{
    class GlobalComponentCache;

    /// FixedCashFlowArguments encapsulates all the parameters
    /// required to create a fixed cash-flow component.
    class FixedCashFlowArguments
    {
    public:

		FixedCashFlowArguments(const LT::date valueDate, 
                               const LT::date setDate,
                               const LT::date payDate,
                               const double rate,
                               const ModuleDate::DayCounterConstPtr basis);
        
		FixedCashFlowArguments(const LT::date valueDate, 
                               const LT::date setDate,
                               const LT::date payDate,
                               const double rate,
                               const ModuleDate::DayCounterConstPtr basis,
                               GlobalComponentCache& globalComponentCache);

        FixedCashFlowArguments(FixedCashFlowArguments const& original, CloneLookup& lookup);
        
        struct Compare
        {
            bool operator()(const FixedCashFlowArguments& lhs, const FixedCashFlowArguments& rhs)
            {
                if(lhs.m_coverage != rhs.m_coverage)  
                    return lhs.m_coverage < rhs.m_coverage;
                if(lhs.m_rate != rhs.m_rate)
                    return lhs.m_rate < rhs.m_rate;
                return DiscountFactorArguments::Compare()(lhs.m_discountFactorArguments, rhs.m_discountFactorArguments);
            }
        };

        DiscountFactorPtr getDiscountFactor() const
        {
            return m_discountFactor;
        }

        double getCoverage() const
        {
            return m_coverage;
        }

        double getRate() const
        {
            return m_rate;
        }
    
        virtual std::ostream& print(std::ostream& out) const;    

    private:
        DiscountFactorArguments m_discountFactorArguments;
        DiscountFactorPtr m_discountFactor;
        double m_coverage;
        double m_rate;
        LT::date m_setDate;   // not strictly necessary, for DEBUG
    };
    
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FIXEDCASHFLOWARGUMENTS_H_INCLUDED