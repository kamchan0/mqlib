/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FORWARDRATEARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FORWARDRATEARGUMENTS_H_INCLUDED
#pragma once
#include "TenorDiscountFactorArguments.h"
#include "GenericInstrumentComponent.h"

namespace ModuleDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
}


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( InstrumentComponent )

    class GlobalComponentCache;

    /// ForwardRateArguments encapsulates the parameters 
    /// required to create a Forward Rate Argument.
    class ForwardRateArguments
    {
    public:
		ForwardRateArguments(const LT::date valueDate,
                             const LT::date fixingDate,
                             const LT::date startDate,
                             const LT::date endDate,
                             const std::string& tenorDescription,
                             const ModuleDate::DayCounterConstPtr basis);

        ForwardRateArguments(const LT::date valueDate,
                             const LT::date fixingDate,
                             const LT::date startDate,
                             const LT::date endDate,
                             const double tenor,
                             const ModuleDate::DayCounterConstPtr basis,
                             GlobalComponentCache& globalComponentCache);
        
        ForwardRateArguments(const LT::date valueDate,
                             const LT::date fixingDate,
                             const LT::date startDate,
                             const LT::date endDate,
                             const std::string& tenorDescription,
                             const ModuleDate::DayCounterConstPtr basis,
                             GlobalComponentCache& globalComponentCache);
        ForwardRateArguments(const LT::date valueDate,
                             const LT::date fixingDate,
                             const LT::date startDate,
                             const LT::date endDate,
                             const std::string& tenorDescription,
                             const ModuleDate::DayCounterConstPtr basis,
                             const  LTQC::Currency& ccy,
                             const LT::Str& index,
                             GlobalComponentCache& globalComponentCache);
         
        ForwardRateArguments(const LT::date valueDate,
                             const LT::date fixingDate,
                             const LT::date startDate,
                             const LT::date endDate,
                             const std::string& tenorDescription,
                             const ModuleDate::DayCounterConstPtr basis,
                             const  LTQC::Currency& ccy,
                             const LT::Str& index);
        
        ForwardRateArguments(ForwardRateArguments const& original, CloneLookup& lookup);

		//	Returns the LTQC::Tenor description (i.e. Tenor as a string)
		inline std::string getTenorDescription() const
		{
			return m_tenorDescription;
		}

		//	Returns the Tenor as a year fraction
		inline double getTenor() const
		{
			return m_tenor;
		}

		//	Returns the coverage of the forward rate
		inline double getCoverage() const
		{
			return m_coverage;
		}

		//	Returns the inverse of the coverage of the forward rate
		inline double getCoverageInverse() const
		{
			return m_coverageInverse;
		}
	 
		//	Returns the time to expiry
		inline double getTimeToExpiry() const
		{
			return m_timeToExpiry;
		}

		//	Returns the time to maturity
		inline double getTimeToMaturity() const
		{
			return m_timeToMaturity;
		}

		//	Returns the fixing date
		inline LT::date getFixingDate() const
		{
			return m_fixingDate;
		}
		
		//	Returns the start date
		inline LT::date getStartDate() const
		{
			return m_startDate;
		}
	       
		//	Returns the end date
		inline LT::date getEndDate() const
		{
			return m_endDate;
		}

        inline double getLastRelevantTime() const
		{
			return m_lastRelevantTime;
		}
        
        ModuleDate::DayCounterConstPtr basis() const
        {
            return m_basis;

        }
		typedef std::tr1::shared_ptr<GenericInstrumentComponent<TenorDiscountFactorArguments> > TenorDiscountFactorPtr;

		//	Returns the start date index discount factor
		inline TenorDiscountFactorPtr getStartDateTenorDiscountFactor() const
		{
			return m_startDateTenorDiscountFactor;
		}
	 
		//	Returns the end date index discount factor
		inline TenorDiscountFactorPtr getEndDateTenorDiscountFactor() const
		{
			return m_endDateTenorDiscountFactor;
		}
   
        
        struct Compare
        {
            bool operator()(const ForwardRateArguments& lhs, const ForwardRateArguments& rhs)
            {
                if(lhs.m_tenor != rhs.m_tenor)
                {
                    return lhs.m_tenor < rhs.m_tenor;
                }
                if(lhs.m_coverage != rhs.m_coverage)
                {
                    return lhs.m_coverage < rhs.m_coverage;
                }

                const TenorDiscountFactorArguments & lhsSetTdfArgs = lhs.m_startDateTenorDiscountFactor->getArguments();
                const TenorDiscountFactorArguments & lhsPayTdfArgs = lhs.m_endDateTenorDiscountFactor->getArguments();
                const TenorDiscountFactorArguments & rhsSetTdfArgs = rhs.m_startDateTenorDiscountFactor->getArguments();
                const TenorDiscountFactorArguments & rhsPayTdfArgs = rhs.m_endDateTenorDiscountFactor->getArguments();

                return (TenorDiscountFactorArguments::Compare()(lhsSetTdfArgs, rhsSetTdfArgs) ||
                        !TenorDiscountFactorArguments::Compare()(rhsSetTdfArgs, lhsSetTdfArgs) && // use of equivalence rather than equality
                            TenorDiscountFactorArguments::Compare()(lhsPayTdfArgs, rhsPayTdfArgs));
            }
        };  //  Compare

        bool operator==(const ForwardRateArguments& other) const
        {
            return *m_startDateTenorDiscountFactor == *(other.m_startDateTenorDiscountFactor) 
                && *m_endDateTenorDiscountFactor == *(other.m_endDateTenorDiscountFactor);
        }

        virtual std::ostream& print(std::ostream& out) const;

    private:
		std::string				m_tenorDescription;
		double					m_tenor;
        double					m_coverage;
        double					m_coverageInverse;

        TenorDiscountFactorPtr	m_startDateTenorDiscountFactor;
        TenorDiscountFactorPtr	m_endDateTenorDiscountFactor;   

        LT::date				m_fixingDate;           // not strictly necessary,
        LT::date				m_startDate;            // not strictly necessary,
        LT::date				m_endDate;				// helpful for DEBUG
        double					m_timeToExpiry;			// for FuturesConvexityModel - could be calculated at 
        double					m_timeToMaturity;		// ForwardRate ctor level, provided we keep the value date
        double                  m_lastRelevantTime;
        ModuleDate::DayCounterConstPtr m_basis;
    };  //  ForwardRateArguments

	size_t hash_value(const ForwardRateArguments& forwardRateArgs);

	namespace 
	{
		std::ostream& operator<<(std::ostream& out, const ForwardRateArguments& forwardRateArguments)
		{
			return forwardRateArguments.print(out);
		}
	}
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FORWARDRATEARGUMENTS_H_INCLUDED