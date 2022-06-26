#ifndef __LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDOISCASHFLOWARGUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DISCOUNTEDOISCASHFLOWARGUMENTS_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "FlexYCFCloneLookup.h"
#include "tenor.h"
#include "Currency.h"

namespace ModuleDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
}


namespace FlexYCF
{
    class GlobalComponentCache;

    class DiscountedOISCashflowArguments
    {
    public:
        DiscountedOISCashflowArguments(const LT::date& valueDate,
									   const LT::date& startDate,
									   const LT::date& endDate,
									   const LTQC::Tenor& payDelay,
									   const LT::Str& payCalendar,
									   const ModuleDate::DayCounterConstPtr basisON,
									   const ModuleDate::DayCounterConstPtr basisCashflow,
                                       const LTQC::Currency& ccy= "", 
                                       const LT::Str& index = "");
        
        double getFlowTime() const;

        struct Compare
        {
            bool operator()(const DiscountedOISCashflowArguments& lhs, const DiscountedOISCashflowArguments& rhs)
            {
                return lhs.m_coverage < rhs.m_coverage ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON < rhs.m_coverageON ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate < rhs.m_payDate ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate < rhs.m_startDate ||
				lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate < rhs.m_endDate ||
				lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate == rhs.m_endDate && lhs.m_ccy < rhs.m_ccy ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate == rhs.m_endDate && lhs.m_ccy.compareCaseless(rhs.m_ccy) == 0 && lhs.m_index < rhs.m_index;
            }
        };

        bool operator==(const DiscountedOISCashflowArguments& other) const
        {
            return (m_startDate == other.m_startDate) && 
                   (m_endDate == other.m_endDate) && 
                   (m_payDate == other.m_payDate) && 
                   (m_valueDate == other.m_valueDate) && 
                   (m_coverageON == other.m_coverageON) && 
                   (m_coverage == other.m_coverage) &&
                   m_ccy.compareCaseless(other.m_ccy) == 0 &&
                   m_index.compareCaseless(other.m_index) == 0;
        }

        LT::date getPayDate() const;
		LT::date getStartDate() const  { return m_startDate;}
		LT::date getEndDate() const    { return m_endDate;  }
		LT::date getValueDate() const  { return m_valueDate;}
		double getCoverage() const { return m_coverage; }
		double getCoverageON() const { return m_coverageON; }
        LTQC::Currency getCurrency() const { return m_ccy; }
        LT::Str getIndex() const { return m_index; }
        
        virtual std::ostream& print(std::ostream& out) const;

		DiscountedOISCashflowArguments(DiscountedOISCashflowArguments const& original, CloneLookup& lookup);
    private:
        LT::date m_startDate;
		LT::date m_endDate;
		LT::date m_valueDate;
		LT::date m_payDate;
        
        double	m_coverageON;
		double	m_coverage;

        LTQC::Currency m_ccy;
        LT::Str        m_index;
    };  

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const DiscountedOISCashflowArguments& oisArguments)
		{
			return oisArguments.print(out);
		}
	}
}


namespace FlexYCF
{
    class GlobalComponentCache;

    class FundingDiscountedCashflowArguments
    {
    public:
        FundingDiscountedCashflowArguments(const LT::date& valueDate,
									   const LT::date& startDate,
									   const LT::date& endDate,
									   const ModuleDate::DayCounterConstPtr basisCashflow,
                                       const LTQC::Currency& ccy= "", 
                                       const LT::Str& index = "");
        
        double getFlowTime() const;

        struct Compare
        {
            bool operator()(const FundingDiscountedCashflowArguments& lhs, const FundingDiscountedCashflowArguments& rhs)
            {
               return lhs.m_coverage < rhs.m_coverage ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_startDate < rhs.m_startDate ||
				lhs.m_coverage == rhs.m_coverage && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate < rhs.m_endDate ||
				lhs.m_coverage == rhs.m_coverage && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate == rhs.m_endDate && lhs.m_ccy < rhs.m_ccy ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate == rhs.m_endDate && lhs.m_ccy.compareCaseless(rhs.m_ccy) == 0 && lhs.m_index < rhs.m_index; 
            }
        };

        bool operator==(const FundingDiscountedCashflowArguments& other) const
        {
            return (m_startDate == other.m_startDate) && 
                (m_endDate == other.m_endDate) && 
                (m_valueDate == other.m_valueDate) && 
                (m_coverage == other.m_coverage) &&
                m_ccy.compareCaseless(other.m_ccy) == 0 &&
                m_index.compareCaseless(other.m_index) == 0;
        }

        LT::date getPayDate() const;
		LT::date getStartDate() const  { return m_startDate;}
		LT::date getEndDate() const    { return m_endDate;  }
		LT::date getValueDate() const  { return m_valueDate;}
		double getCoverage() const { return m_coverage; }
        LTQC::Currency getCurrency() const { return m_ccy; }
        LT::Str getIndex() const { return m_index; }
        virtual std::ostream& print(std::ostream& out) const;

		FundingDiscountedCashflowArguments(FundingDiscountedCashflowArguments const& original, CloneLookup& lookup);
    private:
        LT::date m_startDate;
		LT::date m_endDate;
		LT::date m_valueDate;

		double	m_coverage;

        LTQC::Currency m_ccy;
        LT::Str        m_index;
    };  

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const FundingDiscountedCashflowArguments& oisArguments)
		{
			return oisArguments.print(out);
		}
	}
}

namespace FlexYCF
{
    class GlobalComponentCache;

    class DiscountedArithmeticOISCashflowArguments
    {
    public:
        DiscountedArithmeticOISCashflowArguments(const LT::date& valueDate,
									   const LT::date& startDate,
									   const LT::date& endDate,
									   const LT::Str& accrualCalendar,
									   const LTQC::Tenor& payDelay,
									   const LT::Str& payCalendar,
									   const ModuleDate::DayCounterConstPtr basisON,
									   const ModuleDate::DayCounterConstPtr basisCashflow,
									   size_t cutOff,
                                       const LTQC::Currency& ccy= "", 
                                       const LT::Str& index = "");
        
        double getFlowTime() const;

        struct Compare
        {
            bool operator()(const DiscountedArithmeticOISCashflowArguments& lhs, const DiscountedArithmeticOISCashflowArguments& rhs)
            {
                return lhs.m_coverage < rhs.m_coverage ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON < rhs.m_coverageON ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate < rhs.m_payDate ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate < rhs.m_startDate ||
				lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate < rhs.m_endDate ||
				lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate == rhs.m_endDate && lhs.m_ccy < rhs.m_ccy ||
                lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate == rhs.m_endDate && lhs.m_ccy.compareCaseless(rhs.m_ccy) == 0 && lhs.m_index < rhs.m_index ||
				lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate == rhs.m_endDate && lhs.m_ccy.compareCaseless(rhs.m_ccy) == 0 && lhs.m_index.compareCaseless(rhs.m_index) == 0 && lhs.m_accrualCalendar < rhs.m_accrualCalendar ||
				lhs.m_coverage == rhs.m_coverage && lhs.m_coverageON == rhs.m_coverageON && lhs.m_payDate == rhs.m_payDate && lhs.m_startDate == rhs.m_startDate && lhs.m_endDate == rhs.m_endDate && lhs.m_ccy.compareCaseless(rhs.m_ccy) == 0 && lhs.m_index.compareCaseless(rhs.m_index) == 0 && lhs.m_accrualCalendar.compareCaseless(rhs.m_accrualCalendar) == 0 && lhs.m_cutoff < rhs.m_cutoff;
            }
        };

        bool operator==(const DiscountedArithmeticOISCashflowArguments& other) const
        {
            return (m_startDate == other.m_startDate) && 
                   (m_endDate == other.m_endDate) && 
                   (m_payDate == other.m_payDate) && 
                   (m_valueDate == other.m_valueDate) && 
                   (m_coverageON == other.m_coverageON) && 
                   (m_coverage == other.m_coverage) &&
                   m_ccy.compareCaseless(other.m_ccy) == 0 &&
                   m_index.compareCaseless(other.m_index) == 0 &&
				   m_cutoff == other.m_cutoff;
        }

        LT::date getPayDate() const;
		LT::date getStartDate() const  { return m_startDate;}
		LT::date getEndDate() const    { return m_endDate;  }

		const std::vector<LT::date>& getEndDates() const    { return m_endDates;  }
		double   cutOffAdj() const { return m_cutOffAdj; }

		LT::date getValueDate() const  { return m_valueDate;}
		double getCoverage() const { return m_coverage; }
		double getCoverageON() const { return m_coverageON; }
        LTQC::Currency getCurrency() const { return m_ccy; }
        LT::Str getIndex() const { return m_index; }
        
        virtual std::ostream& print(std::ostream& out) const;

		DiscountedArithmeticOISCashflowArguments(DiscountedArithmeticOISCashflowArguments const& original, CloneLookup& lookup);
    private:
        LT::date m_startDate;
		LT::date m_endDate;
		LT::date m_valueDate;
		LT::date m_payDate;
        size_t   m_cutoff;
		LT::Str				  m_accrualCalendar;
		std::vector<LT::date> m_endDates;
		double				  m_cutOffAdj;

        double	m_coverageON;
		double	m_coverage;

        LTQC::Currency m_ccy;
        LT::Str        m_index;
    };  

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const DiscountedArithmeticOISCashflowArguments& oisArguments)
		{
			return oisArguments.print(out);
		}
	}
}

#endif 