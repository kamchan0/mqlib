/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDFORWARDRATEARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDFORWARDRATEARGUMENTS_H_INCLUDED
#pragma once

#include "ForwardRate.h"
#include "DiscountFactor.h"
#include "Currency.h"

namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
}


namespace FlexYCF
{
    class GlobalComponentCache;


    /// DiscountedForwardRateArguments encapsulates all the parameters
    /// required to create a discounted forward rate component.
    ///
    /// Important Note: DiscountForwardRate in fact represents
    ///  the discounted forward rate times year fraction 'yf' :
    ///   cvg(t, T)  *  F(t, T; tenor)  *  DF(0, T)
    class DiscountedForwardRateArguments
    {
    public:
		DiscountedForwardRateArguments(const LT::date valueDate,
                                       const LT::date fixingDate,
                                       const LT::date startDate,
                                       const LT::date endDate,
                                       const LT::date accStartDate,
                                       const LT::date accEndDate,
                                       const LT::date payDate,
                                       const std::string& tenorDescription,
                                       const ModuleDate::DayCounterConstPtr rateBasis,
                                       const ModuleDate::DayCounterConstPtr accrualBasis);

        DiscountedForwardRateArguments(const LT::date valueDate,
                                       const LT::date fixingDate,
                                       const LT::date startDate,
                                       const LT::date endDate,
                                       const LT::date accStartDate,
                                       const LT::date accEndDate,
                                       const LT::date payDate,
                                       const double tenor,
                                       const ModuleDate::DayCounterConstPtr rateBasis,
                                       const ModuleDate::DayCounterConstPtr accrualBasis,
                                       GlobalComponentCache& globalComponentCache);
        
        DiscountedForwardRateArguments(const LT::date valueDate,
                                       const LT::date fixingDate,
                                       const LT::date startDate,
                                       const LT::date endDate,
                                       const LT::date accStartDate,
                                       const LT::date accEndDate,
                                       const LT::date payDate,
                                       const std::string& tenorDescription,
                                       const ModuleDate::DayCounterConstPtr rateBasis,
                                       const ModuleDate::DayCounterConstPtr accrualBasis,
                                       GlobalComponentCache& globalComponentCache);
        
        DiscountedForwardRateArguments(const LT::date valueDate,
                                       const LT::date fixingDate,
                                       const LT::date startDate,
                                       const LT::date endDate,
                                       const LT::date accStartDate,
                                       const LT::date accEndDate,
                                       const LT::date payDate,
                                       const std::string& tenorDescription,
                                       const ModuleDate::DayCounterConstPtr rateBasis,
                                       const ModuleDate::DayCounterConstPtr accrualBasis,
                                       const LTQC::Currency& ccy,
                                       const LT::Str& index);
        
        DiscountedForwardRateArguments(const LT::date valueDate,
                                       const LT::date fixingDate,
                                       const LT::date startDate,
                                       const LT::date endDate,
                                       const LT::date accStartDate,
                                       const LT::date accEndDate,
                                       const LT::date payDate,
                                       const std::string& tenorDescription,
                                       const ModuleDate::DayCounterConstPtr rateBasis,
                                       const ModuleDate::DayCounterConstPtr accrualBasis,
                                       const LTQC::Currency& ccy,
                                       const LT::Str& index,
                                       GlobalComponentCache& globalComponentCache);

        DiscountedForwardRateArguments(DiscountedForwardRateArguments const& original, CloneLookup& lookup);
        
        /// Returns the inner forward rate
        ForwardRatePtr      getForwardRate()    const;
        
        /// Returns the inner discount factor
        DiscountFactorPtr   getDiscountFactor() const;
        
        /// Returns the accrual period coverage
        double              getCoverage()       const;

        /// Returns the reset period coverage
        double              getResetCoverage()       const;

		struct Compare
        {
            bool operator()(const DiscountedForwardRateArguments& lhs, const DiscountedForwardRateArguments& rhs)
            {
                if(lhs.m_coverage != rhs.m_coverage)  
                    return lhs.m_coverage < rhs.m_coverage;

                return  ( ForwardRateArguments::Compare()(lhs.m_forwardRate->getArguments(), rhs.m_forwardRate->getArguments()) ||
                    (!ForwardRateArguments::Compare()(rhs.m_forwardRate->getArguments(), lhs.m_forwardRate->getArguments()) && 
                        DiscountFactorArguments::Compare()(lhs.m_discountFactor->getArguments(), rhs.m_discountFactor->getArguments()) )
                        );
            }
        };

        bool operator==(const DiscountedForwardRateArguments& other) const
        {
            return *m_forwardRate == *(other.m_forwardRate)
                &&  *m_discountFactor == *(other.m_discountFactor)
				&&	m_coverage == other.m_coverage
				&&  m_accStartDate == other.m_accStartDate
				&&  m_accEndDate == other.m_accEndDate;
				//&&  *m_accrualBasis ==*(other.m_accrualBasis);
        }

        virtual std::ostream& print(std::ostream& out) const;

    private:
        ForwardRatePtr				m_forwardRate;
        DiscountFactorPtr			m_discountFactor;
		double						m_coverage;
		LT::date					m_accStartDate;
		LT::date					m_accEndDate;
		ModuleDate::DayCounterConstPtr m_accrualBasis;
    };  //  DiscountedForwardRateArguments

	size_t hash_value(const DiscountedForwardRateArguments& discountedForwardRateArgs);
	namespace
	{
		std::ostream& operator<<(std::ostream& out, const DiscountedForwardRateArguments& discountedForwardRateArguments)
		{
			return discountedForwardRateArguments.print(out);
		}
	}
}   //  FlexYCF



namespace FlexYCF
{
    class GlobalComponentCache;


    
    class DiscountedForwardRateNotionalExchangeArguments
    {
    public:
		
        
        DiscountedForwardRateNotionalExchangeArguments(const LT::date valueDate,
									   const LT::date spotFxDate,
                                       const LT::date fixingDate,
                                       const LT::date startDate,
                                       const LT::date endDate,
                                       const LT::date accStartDate,
                                       const LT::date accEndDate,
                                       const LT::date payDate,
                                       const std::string& tenorDescription,
                                       const ModuleDate::DayCounterConstPtr rateBasis,
                                       const ModuleDate::DayCounterConstPtr accrualBasis,
                                       const LTQC::Currency& ccy,
                                       const LT::Str& index);
        
        DiscountedForwardRateNotionalExchangeArguments(const LT::date valueDate,
									   const LT::date spotFxDate,
                                       const LT::date fixingDate,
                                       const LT::date startDate,
                                       const LT::date endDate,
                                       const LT::date accStartDate,
                                       const LT::date accEndDate,
                                       const LT::date payDate,
                                       const std::string& tenorDescription,
                                       const ModuleDate::DayCounterConstPtr rateBasis,
                                       const ModuleDate::DayCounterConstPtr accrualBasis,
                                       const LTQC::Currency& ccy,
                                       const LT::Str& index,
                                       GlobalComponentCache& globalComponentCache);

        DiscountedForwardRateNotionalExchangeArguments(DiscountedForwardRateNotionalExchangeArguments const& original, CloneLookup& lookup);
        
        /// Returns the inner forward rate
        ForwardRatePtr      getForwardRate()    const;
        
        /// Returns the inner discount factor
        DiscountFactorPtr   getDiscountFactor() const;
        
        DiscountFactorPtr   getStartDateDiscountFactor() const;
        
        DiscountFactorPtr   getForeignStartDateDiscountFactor() const;
        
        DiscountFactorPtr   getDomesticFixingDateDiscountFactor() const;
        
        DiscountFactorPtr   getForeignFixingDateDiscountFactor() const;
         
		DiscountFactorPtr   getDomesticSpotFxDateDiscountFactor() const;
        
        DiscountFactorPtr   getForeignSpotFxDateDiscountFactor() const;
        /// Returns the accrual period coverage
        double              getCoverage()       const;

        /// Returns the reset period coverage
        double              getResetCoverage()       const;
         
       
		
        struct Compare
        {
            bool operator()(const DiscountedForwardRateNotionalExchangeArguments& lhs, const DiscountedForwardRateNotionalExchangeArguments& rhs)
            {
                if(lhs.m_coverage != rhs.m_coverage)  
                    return lhs.m_coverage < rhs.m_coverage;
                return  ( ForwardRateArguments::Compare()(lhs.m_forwardRate->getArguments(), rhs.m_forwardRate->getArguments()) ||
                    (!ForwardRateArguments::Compare()(rhs.m_forwardRate->getArguments(), lhs.m_forwardRate->getArguments()) && 
                        DiscountFactorArguments::Compare()(lhs.m_discountFactor->getArguments(), rhs.m_discountFactor->getArguments()) )
                        );
            }
        };

        bool operator==(const DiscountedForwardRateNotionalExchangeArguments& other) const
        {
            return *m_forwardRate == *(other.m_forwardRate)
                &&  *m_discountFactor == *(other.m_discountFactor)
				&&	m_coverage == other.m_coverage
				&&  m_accStartDate == other.m_accStartDate
				&&  m_accEndDate == other.m_accEndDate;
        }

        virtual std::ostream& print(std::ostream& out) const;

    private:
        ForwardRatePtr				m_forwardRate;
        DiscountFactorPtr			m_discountFactor;
        DiscountFactorPtr			m_startDateDiscountFactor;
        DiscountFactorPtr			m_foreignStartDateDiscountFactor;
        DiscountFactorPtr			m_domesticFixingDateDiscountFactor;
        DiscountFactorPtr			m_foreignFixingDateDiscountFactor;
		DiscountFactorPtr			m_domesticSpotFxDateDiscountFactor;
        DiscountFactorPtr			m_foreignSpotFxDateDiscountFactor;
		double						m_coverage;
		LT::date					m_accStartDate;
		LT::date					m_accEndDate;
		ModuleDate::DayCounterConstPtr  m_accrualBasis;
    }; 

	size_t hash_value(const DiscountedForwardRateNotionalExchangeArguments& discountedForwardRateArgs);
	namespace
	{
		std::ostream& operator<<(std::ostream& out, const DiscountedForwardRateNotionalExchangeArguments& discountedForwardRateArguments)
		{
			return discountedForwardRateArguments.print(out);
		}
	}
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDFORWARDRATEARGUMENTS_H_INCLUDED