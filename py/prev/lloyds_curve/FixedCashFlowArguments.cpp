/*****************************************************************************
    
	FixedCashFlowArguments

	Implementation of the FixedCashFlowArguments class.
    
	@Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "FixedCashFlowArguments.h"
#include "GlobalComponentCache.h"
#include "FlexYCFCloneLookup.h"

using namespace std;


using namespace LTQC;

namespace FlexYCF
{
	FixedCashFlowArguments::FixedCashFlowArguments(const LT::date valueDate, 
												   const LT::date setDate,
												   const LT::date payDate,
												   const double rate,
												   const ModuleDate::DayCounterConstPtr basis):
		m_discountFactorArguments(DiscountFactor::Arguments(valueDate, payDate)),
		m_discountFactor(DiscountFactor::create(m_discountFactorArguments)),
        m_coverage(basis->getDaysOverBasis(setDate, payDate)),
        m_rate(rate),
        m_setDate(setDate)
	{
	}

    FixedCashFlowArguments::FixedCashFlowArguments(const LT::date valueDate,
                                                   const LT::date setDate,
                                                   const LT::date payDate,
                                                   const double rate,
                                                   const ModuleDate::DayCounterConstPtr basis,
                                                   GlobalComponentCache& globalComponentCache):
        m_discountFactorArguments(DiscountFactor::Arguments(valueDate, payDate)),
        m_discountFactor(globalComponentCache.get(m_discountFactorArguments)),
        m_coverage(basis->getDaysOverBasis(setDate, payDate)),
        m_rate(rate),
        m_setDate(setDate)
    {
    }

    /**
        @brief Pseudo copy constuctor.

        A lookup is used to preserve directed graph relationships.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    FixedCashFlowArguments::FixedCashFlowArguments(FixedCashFlowArguments const& original, CloneLookup& lookup) :
        m_discountFactorArguments(original.m_discountFactorArguments),
        m_discountFactor(lookup.get(original.m_discountFactor)),
        m_coverage(original.m_coverage),
        m_rate(original.m_rate),
        m_setDate(original.m_setDate)
    {
    }

    ostream&  FixedCashFlowArguments::print(ostream& out) const
    {
        out << m_setDate << "  " << m_discountFactorArguments.getPayDate() 
            << ", rate: " << m_rate << ", cvg: " << m_coverage << endl;
        return out;
    }
}