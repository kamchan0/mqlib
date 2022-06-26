#include "stdafx.h"
#include "ImpliedVolQuotes.h"
#include "Data/GenericData.h"

using namespace std;

namespace FlexYCF
{
    void ImpliedVolQuotes::add(const ImpliedVolQuote& impliedVolQuote)
    {
        ImpliedVolQuoteContainer::iterator lower(lower_bound(m_impliedVolQuotes.begin(), 
                                                             m_impliedVolQuotes.end(), 
                                                             impliedVolQuote, 
                                                             ImpliedVolQuote::CompareByExpiry()));
        
        if(lower == m_impliedVolQuotes.end() || ImpliedVolQuote::CompareByExpiry()(impliedVolQuote, *lower))
        {   // insert implied vol quote  at lower position
            m_impliedVolQuotes.insert(lower, impliedVolQuote);
        }
        else
        {   // an implied vol quote with the same time to expiry is already in the implied vol quotes
            LT_THROW_ERROR("Error in 'FlexYCF::ImpliedVolQuotes'. The function 'add' failed because an implied vol quote with time to expiry " 
                << impliedVolQuote.getTimeToExpiry() << " already exists in the impliedVolQuotes.");
        }
    }
    
    void ImpliedVolQuotes::sortByExpiry()
    {
        sort(m_impliedVolQuotes.begin(), m_impliedVolQuotes.end(), ImpliedVolQuote::CompareByExpiry());
    }

    const ImpliedVolQuote& ImpliedVolQuotes::operator[](const size_t index) const
    {
        return m_impliedVolQuotes[index];
    }

    ImpliedVolQuote& ImpliedVolQuotes::operator[](const size_t index)
    {
        return m_impliedVolQuotes[index];
    }

    size_t ImpliedVolQuotes::size() const
    {
        return m_impliedVolQuotes.size();
    }

    bool ImpliedVolQuotes::empty() const
    {
        return m_impliedVolQuotes.empty();
    }

    void ImpliedVolQuotes::clear()
    {
        m_impliedVolQuotes.clear();    
    }
    
    ImpliedVolQuotes::const_iterator ImpliedVolQuotes::begin() const
    {
        return m_impliedVolQuotes.begin();
    }
    
    ImpliedVolQuotes::const_iterator ImpliedVolQuotes::end() const
    {
        return m_impliedVolQuotes.end();
    }
    
    /*
    /// This loads (ATM) implied volatilities
    void loadImpliedVolQuotesFromTable(ImpliedVolQuotes& impliedVolQuotes,
                                       const LTQuant::GenericDataPtr& impliedVolQuotesTable)
    {
        double timeToExpiry, impliedVol;

        // A. A table with implied vols exists, load the quotes!
        for(size_t iVolCnt(0); iVolCnt < impliedVolQuotesTable->numItems() - 1; ++iVolCnt)
        {
            timeToExpiry = impliedVolQuotesTable->get<double>("Time to Expiry", iVolCnt);
            impliedVol = impliedVolQuotesTable->get<double>("Implied Vol", iVolCnt);
            
            impliedVolQuotes.add(ImpliedVolQuote(timeToExpiry, impliedVol));
        }
    }
    */
}