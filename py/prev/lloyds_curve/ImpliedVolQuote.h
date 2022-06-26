/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_IMPLIEDVOLQUOTE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_IMPLIEDVOLQUOTE_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"

namespace FlexYCF
{
    /// Represents an implied volatility quote
    /// Should it have timeToMaturity too? the model used to imply this vol?
    class ImpliedVolQuote
    {
    public:
        /*ImpliedVolQuote(const double timeToExpiry_, const double vol_):
            timeToExpiry(timeToExpiry_),
            vol(vol_)
        {
        }
        */
        ImpliedVolQuote(const LT::date valueDate,
                        const LT::date expiry,
                        const LT::date maturity);

        ImpliedVolQuote(const LT::date valueDate,
                        const LT::date expiry,
                        const LT::date maturity,
                        const double strike);

        LT::date getExpiry() const;
        LT::date getMaturity() const;
        double getTimeToExpiry() const;
        double getTimeToMaturity() const;
        double getVolatility() const;
        double getStrike() const;
        bool useAtmStrike() const;

        void setVolatility(const double volatility);

        /// Nested functor to sort implied vol quotes by their time to expiry
        struct CompareByExpiry
        {
        public:
            bool operator()(const ImpliedVolQuote& lhs, const ImpliedVolQuote& rhs) const
            {
                return operator()(lhs.m_timeToExpiry, rhs.m_timeToExpiry);
            }

            bool operator()(const ImpliedVolQuote& lhs, const double rhs) const
            {
                return operator()(lhs.m_timeToExpiry, rhs);
            }

            bool operator()(const double lhs, const ImpliedVolQuote& rhs) const
            {
                return operator()(lhs, rhs.m_timeToExpiry);
            }
        private:
            bool operator()(const double lhs, const double rhs) const
            {
                return lhs < rhs;
            }
        };  //  CompareByExpiry
    
    private:
        LT::date m_expiry;
        LT::date m_maturity;
        double m_timeToExpiry;
        double m_timeToMaturity;
        double m_vol;
        double m_strike;
        bool m_useAtmStrike;
    };  // ImpliedVolQuote

    DECLARE_SMART_PTRS( ImpliedVolQuote );
}   // FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_IMPLIEDVOLQUOTE_H_INCLUDED