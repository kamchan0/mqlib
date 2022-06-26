#include "stdafx.h"
#include "ImpliedVoLQuote.h"
#include "ModuleDate/InternalInterface/Utils.h"


namespace FlexYCF
{

    ImpliedVolQuote::ImpliedVolQuote(const LT::date valueDate,
                                     const LT::date expiry,
                                     const LT::date maturity):
        m_expiry(expiry),
        m_maturity(maturity),
        m_timeToExpiry(ModuleDate::getYearsBetween(valueDate, expiry)),
        m_timeToMaturity(ModuleDate::getYearsBetween(valueDate, maturity)),
        m_useAtmStrike(true)
    {
    }

    ImpliedVolQuote::ImpliedVolQuote(const LT::date valueDate,
                                     const LT::date expiry,
                                     const LT::date maturity,
                                     const double strike):
        m_expiry(expiry),
        m_maturity(maturity),
        m_timeToExpiry(ModuleDate::getYearsBetween(valueDate, expiry)),
        m_timeToMaturity(ModuleDate::getYearsBetween(valueDate, maturity)),
        m_strike(strike),
        m_useAtmStrike(false)
    {
    }

    LT::date ImpliedVolQuote::getExpiry() const
    {
        return m_expiry;
    }

    LT::date ImpliedVolQuote::getMaturity() const
    {
        return m_maturity;
    }   

    double ImpliedVolQuote::getTimeToExpiry() const
    {
        return m_timeToExpiry;
    }

    double ImpliedVolQuote::getTimeToMaturity() const
    {
        return m_timeToMaturity;
    }

    double ImpliedVolQuote::getVolatility() const
    {
        return m_vol;
    }

    double ImpliedVolQuote::getStrike() const
    {
        return m_strike;
    }

    bool ImpliedVolQuote::useAtmStrike() const
    {
        return m_useAtmStrike;
    }

    void ImpliedVolQuote::setVolatility(const double volatility)
    {
        m_vol = volatility;
    }

}