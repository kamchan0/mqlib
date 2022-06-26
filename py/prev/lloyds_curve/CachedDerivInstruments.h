/*****************************************************************************

    Thin wrapper for providing an un iteration interface over base classes
    when all we have are iterators to derived classes. Boost has been removed
    from QL hence boost::transform iterator is not available
    
    @Mark Ayzenshteyn
    
    Copyright (C) Lloyds TSB Group plc 2011 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CACHEDDERIVINSTRUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CACHEDDERIVINSTRUMENTS_H_INCLUDED
#pragma once

#include <iterator>
#include "CalibrationInstruments.h"
#include "CachedDerivInstrument.h"

namespace FlexYCF
{

    template <typename OuterType,typename InnerIteratorType>
    class SimpleUpcastIterator: public std::iterator<std::bidirectional_iterator_tag,OuterType>
    {
    public:
        SimpleUpcastIterator(InnerIteratorType innerIter) :m_innerIter(innerIter){}

        //postfix
        SimpleUpcastIterator& operator++(int)
        {
            SimpleUpcastIterator it(*this);
            ++*this;
            return it;
        }

        //prefix
        SimpleUpcastIterator& operator++()
        {
            ++m_innerIter;
            return *this;
        }

        friend bool operator== (const SimpleUpcastIterator &lhs, const SimpleUpcastIterator &rhs)
        {
            return lhs.m_innerIter==rhs.m_innerIter;
        }

        friend bool operator!= (const SimpleUpcastIterator &lhs, const SimpleUpcastIterator &rhs)
        {
            return !(lhs==rhs);
        }
        //this implementation forces the data pointed to by the iterator to be a shared_ptr else it will not compile
        //hence returning by value on a dereference op is efficient
        //however this means there is no impl of ->
        //to create an impl of -> change to hold a mutable OuterType::element_type, convert into that
        //and return ptr/ref to it
        typename value_type operator*() const {return std::tr1::static_pointer_cast<OuterType::element_type>(*m_innerIter);}
    private:
       InnerIteratorType m_innerIter;


    };
    class CachedDerivInstruments
    {
        public:
            CachedDerivInstruments(const CalibrationInstruments& calibInstruments):m_calibInstruments(calibInstruments){}
        

            //typedef WrappedIterator<CachedDerivInstrumentPtr,CalibrationInstrumentPtr> iterator;
            typedef SimpleUpcastIterator<CachedDerivInstrumentConstPtr, CalibrationInstruments::const_iterator> const_iterator;

            inline const_iterator begin() const
            {
                return const_iterator(m_calibInstruments.begin());
            }
            inline const_iterator end() const
            {
                return const_iterator(m_calibInstruments.end());
            }
            // Returns the number of instruments
            inline size_t size() const
            {
                return m_calibInstruments.size();
            }

            // Returns the index-th instrument in the container (1st instrument at 0)
            inline CachedDerivInstrumentConstPtr operator[](const size_t index) const
            {
                return std::tr1::static_pointer_cast<const CachedDerivInstrument>(m_calibInstruments[index]);
            }
			
			const CalibrationInstruments& calibrationInstruments() const { return m_calibInstruments; }

        private:
            const CalibrationInstruments& m_calibInstruments;
    };

} //namespace FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_CACHEDDERIVINSTRUMENTS_H_INCLUDED