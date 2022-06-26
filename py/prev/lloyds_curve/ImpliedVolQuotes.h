/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_IMPLIEDVOLQUOTES_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_IMPLIEDVOLQUOTES_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "ImpliedVolQuote.h"
#include "Dictionary.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    
    // typedef std::vector<ImpliedVolQuote> ImpliedVolQuotes;
    
    class ImpliedVolQuotes
    {
    private:
        typedef std::vector<ImpliedVolQuote> ImpliedVolQuoteContainer;
    
    public:
        typedef ImpliedVolQuoteContainer::const_iterator const_iterator;
        
        void add(const ImpliedVolQuote& impliedVolQuote);
        void sortByExpiry();
        const ImpliedVolQuote& operator[](const size_t index) const;
        ImpliedVolQuote& operator[](const size_t index);
        size_t size() const;
        bool empty() const;
        void clear();
        const_iterator begin() const;
        const_iterator end() const;
    private:
        ImpliedVolQuoteContainer m_impliedVolQuotes;
    };  

    /*
    void loadImpliedVolQuotesFromTable(ImpliedVolQuotes& impliedVolQuotes,
                                       const LTQuant::GenericDataPtr& impliedVolQuotesTable);
    */
}

#endif //__LIBRARY_PRICERS_FLEXYCF_IMPLIEDVOLQUOTES_H_INCLUDED