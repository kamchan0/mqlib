/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DICTIONARY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DICTIONARY_H_INCLUDED
#pragma once

namespace FlexYCF
{
    // Represents an ordered collection of (key, value) pairs,
    // each key being unique. Conceptually the same as a map,
    // but may be more efficient when all entries are made first
    // and the structure is then used.
    //
    // TO DO: Rewrite Cache using this Dictionary
    template< class KeyType,
              class ValueType,
              class KeyCompare = std::less<KeyType> >
    class Dictionary
    {
    public:
        typedef std::pair<KeyType, ValueType>                    KeyValuePair;
        typedef std::vector<KeyValuePair>                        KeyValueContainer;
    
    public:
        typedef typename KeyValueContainer::iterator        iterator;
        typedef typename KeyValueContainer::const_iterator  const_iterator;


        struct KeyValueCompare
        {
        public:
            bool operator()(const KeyValuePair& lhs, const KeyValuePair& rhs)
            {
                return operator()(lhs.first, rhs.first);
            }

            bool operator()(const KeyValuePair& lhs, const KeyType& rhs)
            {
                return operator()(lhs.first, rhs);
            }

            bool operator()(const KeyType& lhs, const KeyValuePair& rhs)
            {
                return operator()(lhs, rhs.first);
            }

        private:
            bool operator()(const KeyType& lhs, const KeyType& rhs)
            {
                return KeyCompare()(lhs, rhs);
            }
        };



        /// Returns a const_iterator pointing to the first element in the dictionary
        const_iterator begin() const    { return m_container.begin(); }

        /// Returns a const iterator pointing to one past the last element in the dictionary
        const_iterator end() const      { return m_container.end(); }

        /// Returns an iterator pointing to the first element in the dictionary
        iterator begin()    { return m_container.begin();   }
        
        /// Returns an iterator pointing to one past the last element in the dictionary 
        iterator end()      { return m_container.end();     }

        size_t size() const { return m_container.size();    }

        const_iterator /*getLowerBoundConst*/ lowerBound(const KeyType& key) const
        {
            return lower_bound(m_container.begin(), m_container.end(), key, KeyValueCompare());
        }

        const_iterator upperBound(const KeyType& key) const
        {
            return upper_bound(m_container.begin(), m_container.end(), key, KeyValueCompare());
        }

        /* 
        iterator find(const KeyType& key) const
        {
            iterator lower(lower_bound(m_container.begin(),
                                       m_container.end(),
                                       key,
                                       KeyValueCompare())
                            );

            return (lower != m_container.end() && !KeyValueCompare()(*lower, key) 
        }
        */

        /// Inserts the (key, value) specified in the dictionary if possible
        iterator insert(const KeyType& key, const ValueType& value)
        {
            iterator lower(lower_bound(m_container.begin(),
                                       m_container.end(),
                                       key,
                                       KeyValueCompare())
                            );

            if(lower == m_container.end() || KeyValueCompare()(key, *lower))
            {
                return m_container.insert(lower, KeyValuePair(key, value));
            }
            else
            {
                //LT_THROW_ERROR
                LT_LOG << "A pair with an already existing key was attempted to be added to Dictionary.";
                return m_container.end();
            }
        }

        /// Returns the value associated with the specified key.
        ValueType get(const KeyType& key) const
        {
            const_iterator lower(lowerBound(key));
                                                   
            if(lower == m_container.end() || KeyValueCompare()(key, *lower))
            {
                LT_THROW_ERROR("No entry with this key found.");
            }

            return lower->second;
        }

        /// Returns whether the specified key exists or not.
        bool exists(const KeyType& key) const
        {
            const_iterator lower(lowerBound(key));

            return (lower != end() && !KeyValueCompare()(key, *lower));
        }

    private:
        KeyValueContainer m_container;
    };

}

#endif //__LIBRARY_PRICERS_FLEXYCF_DICTIONARY_H_INCLUDED