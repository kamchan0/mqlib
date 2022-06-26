#include "stdafx.h"
#include "ForwardRateArguments.h"
#include "GlobalComponentCache.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "TenorDiscountFactor.h"
#include "DateUtils.h"
#include "TenorUtils.h"
#include "FlexYCFCloneLookup.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
	ForwardRateArguments::ForwardRateArguments(const LT::date valueDate,
											   const LT::date fixingDate,
											   const LT::date startDate,
											   const LT::date endDate,
											   const std::string& tenorDescription,
											   const ModuleDate::DayCounterConstPtr basis):
		m_tenorDescription(tenorDescription),    
		m_tenor(tenorDescToYears(tenorDescription)),
        m_coverage(basis->getDaysOverBasis(startDate, endDate)),
        // m_coverage(globalComponentCache.getDaysOverBasis(basis, setDate, payDate)), // using DaysOverBasisCache
        m_coverageInverse(1.0 / m_coverage),
        m_startDateTenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(valueDate, startDate, m_tenor))),
        m_endDateTenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(valueDate, endDate, m_tenor))),
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_timeToExpiry(startDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, startDate)),
        m_timeToMaturity(endDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, endDate)),
        m_lastRelevantTime(endDate <= valueDate ? 0.0 : ModuleDate::getYearsBetween(valueDate, endDate)),
        m_basis(basis)
	{
	}

    ForwardRateArguments::ForwardRateArguments(const LT::date valueDate,
											   const LT::date fixingDate,
											   const LT::date startDate,
											   const LT::date endDate,
                                               const double tenor,
                                               const ModuleDate::DayCounterConstPtr basis,
                                               GlobalComponentCache& globalComponentCache) :
		m_tenorDescription(CurveType::getFromYearFraction(tenor)->getDescription()),    
		m_tenor(tenor),
        m_coverage(basis->getDaysOverBasis(startDate, endDate)),
        m_coverageInverse(1.0 / m_coverage),
        m_startDateTenorDiscountFactor( 
            globalComponentCache.get(TenorDiscountFactor::Arguments(valueDate, startDate, m_tenor)))  ,   
        m_endDateTenorDiscountFactor(                                                               // uncomment to call ctors using date calculation caching
            globalComponentCache.get(TenorDiscountFactor::Arguments(valueDate, endDate, m_tenor)))  ,
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_timeToExpiry(startDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, startDate)),
        m_timeToMaturity(endDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, endDate)),
        m_lastRelevantTime(endDate <= valueDate ? 0.0 : ModuleDate::getYearsBetween(valueDate, endDate)),
        m_basis(basis)
    {
    }

    ForwardRateArguments::ForwardRateArguments(const LT::date valueDate,
											   const LT::date fixingDate,
											   const LT::date startDate,
											   const LT::date endDate,
                                               const string& tenorDescription,
                                               const ModuleDate::DayCounterConstPtr basis,
                                               GlobalComponentCache& globalComponentCache):
		m_tenorDescription(tenorDescription),
		m_tenor(tenorDescToYears(tenorDescription)),
        m_coverage(basis->getDaysOverBasis(startDate, endDate)),
        // m_coverage(globalComponentCache.getDaysOverBasis(basis, setDate, payDate)), // using DaysOverBasisCache
        m_coverageInverse(1.0 / m_coverage),
        m_startDateTenorDiscountFactor( 
            globalComponentCache.get(TenorDiscountFactor::Arguments(valueDate, startDate, m_tenor))) ,
        m_endDateTenorDiscountFactor( 
            globalComponentCache.get(TenorDiscountFactor::Arguments(valueDate, endDate, m_tenor))) ,
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_timeToExpiry(startDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, startDate)),
        m_timeToMaturity(endDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, endDate)),
        m_lastRelevantTime(endDate <= valueDate ? 0.0 : ModuleDate::getYearsBetween(valueDate, endDate)),
        m_basis(basis)
    {
    }

    /**
        @brief Pseudo copy constuctor.

        A lookup is used to preserve directed graph relationships.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    ForwardRateArguments::ForwardRateArguments(ForwardRateArguments const& original, CloneLookup& lookup) :
        m_tenorDescription(original.m_tenorDescription),
		m_tenor(original.m_tenor),
        m_coverage(original.m_coverage),
        m_coverageInverse(original.m_coverageInverse),
        m_startDateTenorDiscountFactor(lookup.get(original.m_startDateTenorDiscountFactor)),
        m_endDateTenorDiscountFactor(lookup.get(original.m_endDateTenorDiscountFactor)),   
        m_fixingDate(original.m_fixingDate),
        m_startDate(original.m_startDate),
        m_endDate(original.m_endDate),
        m_timeToExpiry(original.m_timeToExpiry),
        m_timeToMaturity(original.m_timeToMaturity),
        m_lastRelevantTime(original.m_lastRelevantTime),
        m_basis(original.m_basis)
    {
    }

    ForwardRateArguments::ForwardRateArguments(const LT::date valueDate,
											   const LT::date fixingDate,
											   const LT::date startDate,
											   const LT::date endDate,
                                               const string& tenorDescription,
                                               const ModuleDate::DayCounterConstPtr basis,
                                               const  LTQC::Currency& ccy,
                                               const LT::Str& index,
                                               GlobalComponentCache& globalComponentCache):
		m_tenorDescription(tenorDescription),
		m_tenor(tenorDescToYears(tenorDescription)),
        m_coverage(basis->getDaysOverBasis(startDate, endDate)),
        m_coverageInverse(1.0 / m_coverage),
        m_startDateTenorDiscountFactor(globalComponentCache.get(TenorDiscountFactor::Arguments(valueDate, startDate, m_tenor,ccy,index))) ,
        m_endDateTenorDiscountFactor(globalComponentCache.get(TenorDiscountFactor::Arguments(valueDate, endDate, m_tenor,ccy,index))) ,
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_timeToExpiry(startDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, startDate)),
        m_timeToMaturity(endDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, endDate)),
        m_lastRelevantTime(endDate <= valueDate ? 0.0 : ModuleDate::getYearsBetween(valueDate, endDate)),
        m_basis(basis)
    {
    }

    ForwardRateArguments::ForwardRateArguments(const LT::date valueDate,
											   const LT::date fixingDate,
											   const LT::date startDate,
											   const LT::date endDate,
											   const std::string& tenorDescription,
											   const ModuleDate::DayCounterConstPtr basis,
                                               const  LTQC::Currency& ccy,
                                               const LT::Str& index):
		m_tenorDescription(tenorDescription),    
		m_tenor(tenorDescToYears(tenorDescription)),
        m_coverage(basis->getDaysOverBasis(startDate, endDate)),
        m_coverageInverse(1.0 / m_coverage),
        m_startDateTenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(valueDate, startDate, m_tenor,ccy,index))),
        m_endDateTenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(valueDate, endDate, m_tenor,ccy,index))),
        m_fixingDate(fixingDate),
        m_startDate(startDate),
        m_endDate(endDate),
        m_timeToExpiry(startDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, startDate)),
        m_timeToMaturity(endDate <= valueDate ? 0.0 : basis->getDaysOverBasis(valueDate, endDate)),
        m_lastRelevantTime(endDate <= valueDate ? 0.0 : ModuleDate::getYearsBetween(valueDate, endDate)),
        m_basis(basis)
	{
	}

    ostream& ForwardRateArguments::print(ostream& out) const
    {
        out << "Fwd Rate Args";
        return out;
    }
}