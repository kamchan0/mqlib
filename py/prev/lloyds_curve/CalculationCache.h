/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_HASHCACHE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_HASHCACHE_H_INCLUDED
#pragma once


#include <boost/functional/hash.hpp>
#if ( _MSC_VER >= 1500 )					// we will be on Boost_1_38 with VS 2008
	#include "Boost\unordered_map.hpp"
#else
	#include "Boost_1_38_Hack\unordered_map.hpp"
#endif

namespace FlexYCF
{
    /// This template class caches calculations using a hash table.
    /// It uses some boost .hpp files that were copied here because
    /// they have been released recently and are not in the boost version used.
    /// Minor adjustments to #include's have been done.
    template< class Key,
              class Value,
              class Hash  = boost::hash<Key>,
              class Equal = std::equal_to<Key> >
    class CalculationCache
    {
    private:
        typedef boost::function1<Value, const Key>              Function;
        typedef boost::unordered_map<Key, Value, Hash, Equal>   HashTable;
        typedef std::pair<Key, Value>                                KeyValuePair;
    
    public:
        explicit CalculationCache(const size_t minNumberOfBuckets = 50):
            m_hashTable(minNumberOfBuckets)
        {
        }

        explicit CalculationCache(const Function& function,
                                  const size_t minNumberOfBuckets = 50):
            m_function(function),
            m_hashTable(minNumberOfBuckets)
        {
        }

        /// Sets the function for which to cache calculations
        void setFunction(const Function& function)
        {
            m_function = function;
        }
            
        /// Returns the value of the function evaluated at the
        /// specified key, caching if it has not beed calculated
        /// before.
        const Value& get(const Key& key) 
        {
            m_iter = m_hashTable.find(key);
            if(m_iter == m_hashTable.end())
            {
                m_iter = m_hashTable.insert(KeyValuePair(key, m_function(key))).first;
            }
            return m_iter->second;
        }

        /// Set the number of buckets of the hash table
        void rehash(const size_t minNumberOfBuckets)
        {
            m_hashTable.rehash(minNumberOfBuckets);    
        }

        /// Sets the maximum load factor (see
        /// boost documentation for more info)
        void max_load_factor(const float maxLoadFactor)
        {
            m_hashTable.max_load_factor(maxLoadFactor);
        }

        /// Clears the hash table
        void clear()
        {
            m_hashTable.clear();
        }

    private:
        HashTable   m_hashTable;
        Function    m_function;
        typename HashTable::const_iterator m_iter;
    }; // CalculationCache
}
#endif //__LIBRARY_PRICERS_FLEXYCF_HASHCACHE_H_INCLUDED