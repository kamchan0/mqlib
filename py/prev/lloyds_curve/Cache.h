/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CACHE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CACHE_H_INCLUDED
#pragma once

#include <functional>

namespace FlexYCF
{
    /**
        @brief A storage implementation for the FlexYCF cache that uses std::map.

        This class is intended to be used with FlexYCF::Cache.
    */
    template< typename Key,
              typename Value,
              class KeyCompare >
    class CacheStorageMap
    {
    private:
        typedef std::map<Key, Value, KeyCompare> Container;

    public:
        /**
            @brief Test if a key exists?

            @param key The key to test for.
            @return    Returns true if the key exists.
        */
        bool exists(Key const& key) const
        {
            return m_container.find(key) != m_container.end();
        }

        /**
            @brief Create a new key in the storage or retrieve existing value.

            This function will either insert a new key into the container (and default constructed value) or retrieve
            the existing value for the key. The validity of the returned reference is temporary and should be considered 
            invalid once another call to this class is made (c.f. validity of iterators).

            @param key           The key to insert/retrieve.
            @param[out] inserted Whether the key had to be inserted and the value is newly created.
            @return              Returns a reference to the value associated with the key.
        */
        Value& createOrRetrieve(Key const& key, bool& inserted) 
        {
            std::pair<Container::iterator, bool> result;
            result = m_container.insert(Container::value_type(key, Value()));
            inserted = result.second;
            return result.first->second;
        }

        /**
            @brief Clear all stored values.
        */
        void clear()
        {
            m_container.clear();
        }

    private:
        Container m_container;
    };

    /**
        @brief A storage implementation for the FlexYCF cache that uses ordered std::vector.

        This class is intended to be used with FlexYCF::Cache.
    */
    template< typename Key,
              typename Value,
              class KeyCompare >
    class CacheStorageVector
    {
    private:
        typedef std::pair<Key, Value> KeyValuePair;
        typedef std::vector<KeyValuePair> Container;

        /**
            @brief A functional class to compare (key, value) pairs using only their keys.
        */
        struct KeyValueCompare
        {
            KeyCompare m_keyCompare;

            bool operator()(KeyValuePair const& lhs, KeyValuePair const& rhs)
            {
                return m_keyCompare(lhs.first, rhs.first);
            }
        };

    public:
        CacheStorageVector()
        {
            m_iter = m_container.end();
        }

        /**
            @brief Test if a key exists?

            @param key The key to test for.
            @return    Returns true if the key exists.
        */
        bool exists(Key const& key) const
        {
            return std::binary_search(KeyValuePair(key, Value(), m_keyValueCompare));
        }

        /**
            @brief Create a new key in the storage or retrieve existing value.

            This function will either insert a new key into the container (and default constructed value) or retrieve
            the existing value for the key. The validity of the returned reference is temporary and should be considered 
            invalid once another call to this class is made (c.f. validity of iterators).

            @param key           The key to insert/retrieve.
            @param[out] inserted Whether the key had to be inserted and the value is newly created.
            @return              Returns a reference to the value associated with the key.
        */
        Value& createOrRetrieve(Key const& key, bool& inserted) 
        {
            // Look for the iterator that is on just after this key
            KeyValuePair const target(key, Value());
            m_iter = std::lower_bound(m_container.begin(), m_container.end(), target, m_keyValueCompare);

            // If we're less than the iterator found then the key is missing so insert a new value
            inserted = false;
            if ((m_iter == m_container.end()) || m_keyValueCompare(target, *m_iter))
            {
                inserted = true;
                m_iter = m_container.insert(m_iter, target);
            }

            return (*m_iter).second;
        }

        /**
            @brief Clear all stored values.
        */
        void clear()
        {
            m_container.clear();
            m_iter = m_container.end();
        }

    private:
        Container m_container;
        typename Container::iterator m_iter;
        KeyValueCompare m_keyValueCompare;
    };

    /**
        @brief A cache used by FlexYCF components.

        A cache for (key, value) pairs. Storage for the cache is implemented externally to this class
        to allow the performance to be tuned for different situations. The following restrictions are 
        placed on the types supported by this class. The user must supply a functor to create values
        from keys. Values must have default constructors. Keys must support comparison operators.
    */
    template< typename Key,
              typename Value,
              class KeyCompare = std::less<Key>,
              class Storage = CacheStorageMap<Key, Value, KeyCompare>,
              class ValueCreator = std::tr1::function<Value (Key)>>
    class Cache
    {
    public:
        /**
            @brief Default construction implies default constructor value creator.
        */
        Cache()
        {
        }

        /**
            @brief Construct a cache using a specific value creator.

            @param valueCreator The value creator to use.
        */
        explicit Cache(ValueCreator const& valueCreator)
        : m_valueCreator(valueCreator)
        {
        }

        /**
            @brief Set the value creator to use.

            This implicitly clears the cache as existing cached values are now invalid.

            @param valueCreator The value creator to use.    
        */
        void setFunction(ValueCreator const& valueCreator)
        {
            m_valueCreator = valueCreator;
            clear();
        }

        /**
            @brief Test if a key exists in the cache.

            @param key The key to look for.
            @return    Returns true if the key exists.
        */
        bool exists(const Key& key) const
        {
            return m_storage.exists(key);
        }

        /**
            @brief Get the value corresponding to a key.

            If there is no value currently associated with the key then insert the key and associate a new value.

            @param key The key to look up.
            @return    The corresponding value.
        */
        Value get(const Key& key) 
        {
            bool inserted;
            Value& value = m_storage.createOrRetrieve(key, inserted);
            if (inserted)
                value = m_valueCreator(key);
            return value;
        }

        /**
            @brief Clear all cached values.
        */
        void clear()
        {
            m_storage.clear();
        }

    private:
        Storage m_storage;
        ValueCreator m_valueCreator;
    };  //  Cache
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_CACHE_H_INCLUDED