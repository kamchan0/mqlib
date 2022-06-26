/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "CurveAlgorithm.h"

namespace FlexYCF
{
    void CurveAlgorithm::setExtremalIterators(const const_iterator beginIterator, const const_iterator endIterator)
    {

        m_begin = beginIterator;
        m_end   = endIterator;

        onExtremalIteratorsSet();
    }

    size_t CurveAlgorithm::size() const
    {
        return std::distance<const_iterator>(m_begin, m_end);    
    }
}   //  FlexYCF