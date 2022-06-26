#include "stdafx.h"
#include "LTQuantInitial.h"
#include "DiscountedForwardRateArguments.h"
#include "GlobalComponentCache.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "ForwardRate.h"
#include "DiscountFactor.h"
#include "DateUtils.h"
#include "FlexYCFCloneLookup.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;

	DiscountedForwardRateArguments::DiscountedForwardRateArguments(const LT::date valueDate,
																   const LT::date fixingDate,
																   const LT::date startDate,
																   const LT::date endDate,
																   const LT::date accStartDate,
																   const LT::date accEndDate,
																   const LT::date payDate,
																   const std::string& tenorDescription,
																   const DayCounterConstPtr rateBasis,
																   const DayCounterConstPtr accrualBasis):
		m_forwardRate(ForwardRate::create(ForwardRateArguments(valueDate, fixingDate, startDate, endDate, tenorDescription, rateBasis))),
		m_discountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, payDate))), 
		m_accStartDate(accStartDate), m_accEndDate(accEndDate), m_accrualBasis(accrualBasis), m_coverage(accrualBasis->getDaysOverBasis(accStartDate, accEndDate))
	{
	}

    DiscountedForwardRateArguments::DiscountedForwardRateArguments(const LT::date valueDate,
																   const LT::date fixingDate,
																   const LT::date startDate,
																   const LT::date endDate,
																   const LT::date accStartDate,
																   const LT::date accEndDate,
                                                                   const LT::date payDate,
                                                                   const double tenor,
																   const DayCounterConstPtr rateBasis,
																   const DayCounterConstPtr accrualBasis,
                                                                   GlobalComponentCache& globalComponentCache):
        m_forwardRate( globalComponentCache.get(ForwardRate::Arguments(valueDate, fixingDate, startDate, endDate, tenor, rateBasis, globalComponentCache)) ),
        m_discountFactor( globalComponentCache.get(DiscountFactor::Arguments(valueDate, payDate)) ),
		m_accStartDate(accStartDate), m_accEndDate(accEndDate), m_accrualBasis(accrualBasis), m_coverage(accrualBasis->getDaysOverBasis(accStartDate, accEndDate))
    {
    }

    DiscountedForwardRateArguments::DiscountedForwardRateArguments(const LT::date valueDate,
																   const LT::date fixingDate,
																   const LT::date startDate,
																   const LT::date endDate,
																   const LT::date accStartDate,
																   const LT::date accEndDate,
                                                                   const LT::date payDate,
                                                                   const string& tenorDescription,
																   const DayCounterConstPtr rateBasis,
																   const DayCounterConstPtr accrualBasis,
                                                                   GlobalComponentCache& globalComponentCache):
        m_forwardRate( globalComponentCache.get(ForwardRate::Arguments(valueDate, fixingDate, startDate, endDate, tenorDescription, rateBasis, globalComponentCache)) ),
        m_discountFactor( globalComponentCache.get(DiscountFactor::Arguments(valueDate, payDate)) ),
		m_accStartDate(accStartDate), m_accEndDate(accEndDate), m_accrualBasis(accrualBasis), m_coverage(accrualBasis->getDaysOverBasis(accStartDate, accEndDate))
    {
    }

    DiscountedForwardRateArguments::DiscountedForwardRateArguments(const LT::date valueDate,
																   const LT::date fixingDate,
																   const LT::date startDate,
																   const LT::date endDate,
																   const LT::date accStartDate,
																   const LT::date accEndDate,
																   const LT::date payDate,
																   const std::string& tenorDescription,
																   const DayCounterConstPtr rateBasis,
																   const DayCounterConstPtr accrualBasis,
                                                                   const LTQC::Currency& ccy,
                                                                   const LT::Str& index):
		m_forwardRate(ForwardRate::create(ForwardRateArguments(valueDate, fixingDate, startDate, endDate, tenorDescription, rateBasis, ccy, index))),
		m_discountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, payDate, ccy, index))), 
		m_accStartDate(accStartDate), m_accEndDate(accEndDate), m_accrualBasis(accrualBasis), m_coverage(accrualBasis->getDaysOverBasis(accStartDate, accEndDate))
	{
	}

   DiscountedForwardRateArguments::DiscountedForwardRateArguments(const LT::date valueDate,
																   const LT::date fixingDate,
																   const LT::date startDate,
																   const LT::date endDate,
																   const LT::date accStartDate,
																   const LT::date accEndDate,
                                                                   const LT::date payDate,
                                                                   const string& tenorDescription,
																   const DayCounterConstPtr rateBasis,
																   const DayCounterConstPtr accrualBasis,
                                                                   const LTQC::Currency& ccy,
                                                                   const LT::Str& index,
                                                                   GlobalComponentCache& globalComponentCache):
        m_forwardRate( globalComponentCache.get(ForwardRate::Arguments(valueDate, fixingDate, startDate, endDate, tenorDescription, rateBasis, ccy, index, globalComponentCache)) ),
        m_discountFactor( globalComponentCache.get(DiscountFactor::Arguments(valueDate, payDate,ccy,index)) ),
		m_accStartDate(accStartDate), m_accEndDate(accEndDate), m_accrualBasis(accrualBasis), m_coverage(accrualBasis->getDaysOverBasis(accStartDate, accEndDate))
    {
    }
     
    /**
        @brief Pseudo copy constuctor.

        A lookup is used to preserve directed graph relationships.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    DiscountedForwardRateArguments::DiscountedForwardRateArguments(DiscountedForwardRateArguments const& original, CloneLookup& lookup) :
        m_forwardRate(lookup.get(original.m_forwardRate)),
        m_discountFactor(lookup.get(original.m_discountFactor)),
		m_coverage(original.m_coverage),
		m_accStartDate(original.m_accStartDate),
		m_accEndDate(original.m_accEndDate),
		m_accrualBasis(original.m_accrualBasis)
    {
    }

    ForwardRatePtr DiscountedForwardRateArguments::getForwardRate() const
    {
        return m_forwardRate;
    }

    DiscountFactorPtr DiscountedForwardRateArguments::getDiscountFactor() const
    {
        return m_discountFactor;
    }

    double DiscountedForwardRateArguments::getCoverage() const
    {
        return m_coverage;
    }

    double DiscountedForwardRateArguments::getResetCoverage() const
    {
        return m_forwardRate->getArguments().getCoverage();
    }
	
	ostream& DiscountedForwardRateArguments::print(ostream& out) const
    {
        out << m_forwardRate->getStartDate() << " " << m_forwardRate->getEndDate() << endl;
        //m_forwardRate->print(out);
        //m_discountFactor->print(out);
        return out;
    }
}   //  FlexYCF

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;

	

    DiscountedForwardRateNotionalExchangeArguments::DiscountedForwardRateNotionalExchangeArguments(const LT::date valueDate,
																   const LT::date spotFxDate,
																   const LT::date fixingDate,
																   const LT::date startDate,
																   const LT::date endDate,
																   const LT::date accStartDate,
																   const LT::date accEndDate,
																   const LT::date payDate,
																   const std::string& tenorDescription,
																   const DayCounterConstPtr rateBasis,
																   const DayCounterConstPtr accrualBasis,
                                                                   const LTQC::Currency& ccy,
                                                                   const LT::Str& index):
		m_forwardRate(ForwardRate::create(ForwardRateArguments(valueDate, fixingDate, startDate, endDate, tenorDescription, rateBasis, ccy, index))),
		m_discountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, payDate, ccy, index))),
        m_startDateDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, startDate, ccy, index))),
        m_foreignStartDateDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, startDate))),
        m_domesticFixingDateDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, startDate, ccy, index))),
        m_foreignFixingDateDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, startDate))),
		m_domesticSpotFxDateDiscountFactor(DiscountFactor::create(DiscountFactor::Arguments(valueDate, spotFxDate, ccy, index))),
        m_foreignSpotFxDateDiscountFactor(DiscountFactor::create(DiscountFactor::Arguments(valueDate, spotFxDate))),
		m_accStartDate(accStartDate), m_accEndDate(accEndDate), m_accrualBasis(accrualBasis), m_coverage(accrualBasis->getDaysOverBasis(accStartDate, accEndDate))
	{
	}

   DiscountedForwardRateNotionalExchangeArguments::DiscountedForwardRateNotionalExchangeArguments(const LT::date valueDate,
																   const LT::date spotFxDate,
																   const LT::date fixingDate,
																   const LT::date startDate,
																   const LT::date endDate,
																   const LT::date accStartDate,
																   const LT::date accEndDate,
                                                                   const LT::date payDate,
                                                                   const string& tenorDescription,
																   const DayCounterConstPtr rateBasis,
																   const DayCounterConstPtr accrualBasis,
                                                                   const LTQC::Currency& ccy,
                                                                   const LT::Str& index,
                                                                   GlobalComponentCache& globalComponentCache):
        m_forwardRate( globalComponentCache.get(ForwardRate::Arguments(valueDate, fixingDate, startDate, endDate, tenorDescription, rateBasis, ccy, index, globalComponentCache)) ),
        m_discountFactor( globalComponentCache.get(DiscountFactor::Arguments(valueDate, payDate,ccy,index)) ),
        m_startDateDiscountFactor(globalComponentCache.get(DiscountFactor::Arguments(valueDate, startDate, ccy, index))),
        m_foreignStartDateDiscountFactor(globalComponentCache.get(DiscountFactor::Arguments(valueDate, startDate))),
        m_domesticFixingDateDiscountFactor(globalComponentCache.get(DiscountFactor::Arguments(valueDate, startDate, ccy, index))),
        m_foreignFixingDateDiscountFactor(globalComponentCache.get(DiscountFactor::Arguments(valueDate, startDate))),
		m_domesticSpotFxDateDiscountFactor(globalComponentCache.get(DiscountFactor::Arguments(valueDate, spotFxDate, ccy, index))),
        m_foreignSpotFxDateDiscountFactor(globalComponentCache.get(DiscountFactor::Arguments(valueDate, spotFxDate))),
		m_accStartDate(accStartDate), m_accEndDate(accEndDate), m_accrualBasis(accrualBasis), m_coverage(accrualBasis->getDaysOverBasis(accStartDate, accEndDate))
    {
    }
     
   
    DiscountedForwardRateNotionalExchangeArguments::DiscountedForwardRateNotionalExchangeArguments(DiscountedForwardRateNotionalExchangeArguments const& original, CloneLookup& lookup) :
        m_forwardRate(lookup.get(original.m_forwardRate)),
        m_discountFactor(lookup.get(original.m_discountFactor)),
        m_startDateDiscountFactor(lookup.get(original.m_startDateDiscountFactor)),
        m_foreignStartDateDiscountFactor(lookup.get(original.m_startDateDiscountFactor)),
        m_domesticFixingDateDiscountFactor(lookup.get(original.m_domesticFixingDateDiscountFactor)),
        m_foreignFixingDateDiscountFactor(lookup.get(original.m_foreignFixingDateDiscountFactor)),
		m_domesticSpotFxDateDiscountFactor(lookup.get(original.m_domesticSpotFxDateDiscountFactor)),
        m_foreignSpotFxDateDiscountFactor(lookup.get(original.m_foreignSpotFxDateDiscountFactor)),
		m_coverage(original.m_coverage),
		m_accStartDate(original.m_accStartDate),
		m_accEndDate(original.m_accEndDate),
		m_accrualBasis(original.m_accrualBasis)
    {
    }

    ForwardRatePtr DiscountedForwardRateNotionalExchangeArguments::getForwardRate() const
    {
        return m_forwardRate;
    }

    DiscountFactorPtr DiscountedForwardRateNotionalExchangeArguments::getDiscountFactor() const
    {
        return m_discountFactor;
    }
    
    DiscountFactorPtr DiscountedForwardRateNotionalExchangeArguments::getStartDateDiscountFactor() const
    {
        return m_startDateDiscountFactor;
    }
    
    DiscountFactorPtr DiscountedForwardRateNotionalExchangeArguments::getForeignStartDateDiscountFactor() const
    {
        return m_foreignStartDateDiscountFactor;
    }
    
    DiscountFactorPtr DiscountedForwardRateNotionalExchangeArguments::getDomesticFixingDateDiscountFactor() const
    {
        return m_domesticFixingDateDiscountFactor;
    }
    
    DiscountFactorPtr DiscountedForwardRateNotionalExchangeArguments::getForeignFixingDateDiscountFactor() const
    {
        return m_foreignFixingDateDiscountFactor;
    }
	 
	DiscountFactorPtr DiscountedForwardRateNotionalExchangeArguments::getDomesticSpotFxDateDiscountFactor() const
    {
        return m_domesticSpotFxDateDiscountFactor;
    }
    
    DiscountFactorPtr DiscountedForwardRateNotionalExchangeArguments::getForeignSpotFxDateDiscountFactor() const
    {
        return m_foreignSpotFxDateDiscountFactor;
    }
    double DiscountedForwardRateNotionalExchangeArguments::getCoverage() const
    {
        return m_coverage;
    }

    double DiscountedForwardRateNotionalExchangeArguments::getResetCoverage() const
    {
        return m_forwardRate->getArguments().getCoverage();
    }
	
	ostream& DiscountedForwardRateNotionalExchangeArguments::print(ostream& out) const
    {
        out << m_forwardRate->getStartDate() << " " << m_forwardRate->getEndDate() << endl;
        return out;
    }
}   //  FlexYCF