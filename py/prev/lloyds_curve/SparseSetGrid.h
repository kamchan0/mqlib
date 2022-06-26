/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SPARSESETGRID_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SPARSESETGRID_H_INCLUDED
#pragma once

#include "CurveType.h"

namespace FlexYCF
{
    /// SparseSetGrid represents a (XType, YType)-indexed sparse grid
    /// with values of type set<ValueType> XLess and YLess are ways to
    /// sort them (can be useful for shared_pointers)
    template< typename XType, 
              typename YType, 
              typename ValueType,
              typename XLess = std::less<XType>,
              typename YLess = std::less<YType>,
              typename VLess = std::less<ValueType> >
    class SparseSetGrid
    {
    public:
        typedef typename std::set<XType, XLess> XSet;
        typedef typename std::set<YType, YLess> YSet;
        typedef typename std::set<ValueType, VLess> ValueSet;
        typedef typename XSet::const_iterator const_x_iterator;
        typedef typename XSet::iterator x_iterator;
        typedef typename YSet::const_iterator const_y_iterator;
        typedef typename YSet::iterator y_iterator;
        typedef typename std::map<XType, ValueSet > XSetMap;
        typedef typename std::map<YType, ValueSet > YSetMap;
        

        explicit SparseSetGrid()
        {
        }

        const XSet getXHeadings() const
        {
            return m_xHeadings;
        }

        const YSet getYHeadings() const
        {
            return m_yHeadings;
        }

        const_x_iterator x_begin() const
        {
            return m_xHeadings.begin();
        }

        x_iterator x_begin()
        {
            return m_xHeadings.begin();
        }

        typename const_y_iterator y_begin() const
        {
            return m_yHeadings.begin();
        }

        y_iterator y_begin()
        {
            return m_yHeadings.begin();
        }

        const_x_iterator x_end() const
        {
            return m_xHeadings.end();
        }

        x_iterator x_end()
        {
            return m_xHeadings.end();
        }

        const_y_iterator y_end() const
        {
            return m_yHeadings.end();
        }

        y_iterator y_end()
        {
            return  m_yHeadings.end();
        }
        
        // returns the total number of sets in the grid
        const size_t set_count() const
        {
            long setCount(0);
            for(XSet::const_iterator xIter(x_begin()); xIter != x_end(); ++xIter)
            {
                setCount += x_set_count(*xIter);
            }
            return setCount;
        }

        // The number of sets on the x axis
        const long x_set_count(const XType& x) const
        {
            if(m_xHeadings.find(x) == m_xHeadings.end())
            {
                return 0;
            }
            long count(0);
            for(const_y_iterator yIter(m_yHeadings.begin()); yIter != m_yHeadings.end(); ++yIter)
            {
                if(m_map.find(make_pair<XType, YType>(x, *yIter)) != m_map.end())
                {
                    ++count;
                }
            }
            return count;
        }
        
         // The number of sets on the y axis
        const long y_set_count(const YType& y) const
        {
            if(m_yHeadings.find(y) == m_yHeadings.end())
            {
                return 0;
            }
            long count(0);
            for(const_x_iterator xIter(m_xHeadings.begin()); xIter != m_xHeadings.end(); ++xIter)
            {
                if(m_map.find(make_pair<XType, YType>(*xIter, y)) != m_map.end())
                {
                    ++count;
                }
            }
            return count;
        }

        // The number of elements in the set at (x, y)
        const long count(const XType& x, const YType& y) const
        {
            const XSetMap xSetMap(getXSetMap(y));
            XSetMap::const_iterator xSetMapIter(xSetMap.find(x));
            if(xSetMapIter == xSetMap.end())
            {
                return 0;
            }
            else
            {
                return xSetMapIter->second.count();
            }
        }

        const YSetMap getYSetMap(const XType& x)
        {
            map<YType, ValueSet > xSetMap;
            for(const_y_iterator yIter(m_yHeadings.begin()); yIter != m_yHeadings.end(); ++yIter)
            {
                const YType y(*yIter);
                if(m_map.find(make_pair<XType, YType>(x, y)) != m_map.end())
                {
                    xSetMap[y] = m_map[make_pair<XType, YType>(x, y)];
                }
            }
            return xSetMap;
        }

        const XSetMap getXSetMap(const YType & y)
        {
            map<XType, ValueSet > ySetMap;
            for(const_x_iterator xIter(m_xHeadings.begin()); xIter != m_xHeadings.end(); ++xIter)
            {
                const XType x(*xIter);
                if(m_map.find(make_pair<XType, YType>(x, y)) != m_map.end())
                {
                    ySetMap[x] = m_map[make_pair<XType, YType>(x, y)];
                }
            }
            return ySetMap;
        }

        // Add value to the set at location (x,y) of the grid
        void add(const XType& x, const YType& y, const ValueType& value)
        {
            //if(m_xHeadings.find(x) == m_xHeadings.end())
            m_xHeadings.insert(x);
            m_yHeadings.insert(y);
            m_map[make_pair<XType, YType>(x,y)].insert(value); 
        }

        // Remove the value from the the set of location (x, y) of the grid
        // erase the set if it is empty after the operation
        void remove(const XType& x, const YType& y, const ValueType& value)
        {
            const pair<XType, YType> key(x,y);

            map<pair<XType, YType>, set<ValueType> >::iterator iter = m_map.find(key);
           
            if(iter != m_map.end())
            {
                m_map[key].erase(value);
                if(m_map[key].empty())
                {
                    m_map.erase(key);
                }
            }
        }

        // Remove the whole set at location (x, y) of the grid
        void remove(const XType& x, const YType& y)
        {
            const pair<XType, YType> key(x, y);

            map<pair<XType, YType>, set<ValueType> >::iterator iter = m_map.find(key);

            if(iter != m_map.end())
            {
                m_map[key].clear();
                m_map.erase(key);
            }
        }

        void clear()
        {
            m_xHeadings.clear();
            m_yHeadings.clear();
            m_map.clear();
        }

        
        virtual std::ostream& print(std::ostream& out) //const 
        { 
            for(const_x_iterator xIter(m_xHeadings.begin()); xIter != m_xHeadings.end(); ++xIter)
            {
                out << "\t" << (*xIter);
            }
            out << std::endl;

            for(const_y_iterator yIter(m_yHeadings.begin()); yIter != m_yHeadings.end(); ++yIter)
            {
                out << (*yIter);
                const XSetMap xSetMap(getXSetMap(*yIter));

                for(const_x_iterator xIter(m_xHeadings.begin()); xIter != m_xHeadings.end(); ++xIter)
                {
                    XSetMap::const_iterator setIter(xSetMap.find(*xIter));

                    out << "\t";

                    if(setIter != xSetMap.end())
                    {
                        const ValueSet valueSet(setIter->second);
                        ValueSet::const_iterator vIter(valueSet.begin());
                        
                        out << (*vIter);
                        
                        for(++vIter; vIter != valueSet.end(); ++vIter)
                        {
                            out << ", " << (*vIter);
                        }
                    }
                }
                out << std::endl;
            }
            
            return out; 
        }   // print
        

    private:
        std::map<std::pair<XType, YType>, ValueSet > m_map;
        XSet m_xHeadings;
        YSet m_yHeadings;

    };  //   SparseSetGrid

    struct CurveTypeCompare
    {
        bool operator()(const FlexYCF::CurveTypeConstPtr lhs, const FlexYCF::CurveTypeConstPtr rhs) const
        {
            return FlexYCF::CurveType::LessThan(*lhs, *rhs);    
        }
    };

}   // FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_SPARSESETGRID_H_INCLUDED