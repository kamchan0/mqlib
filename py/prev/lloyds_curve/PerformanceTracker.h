/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_PERFORMANCETRACKER_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_PERFORMANCETRACKER_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"

namespace FlexYCF
{
    /// Collects performance information and provides
    /// basic statistics.
    class PerformanceTracker
    {
    private:
        typedef std::pair<std::string, std::vector<double> >   KeyValuePair;
        typedef std::vector<KeyValuePair>            Container;

    public:
        bool exists(const std::string& key) const;
        void insert(const std::string& key);
        std::vector<double>& operator[](const std::string& key);
        const std::vector<double>& operator[](const std::string& key) const;
        std::vector<double>& get(const std::string& key);
        const std::vector<double>& get(const std::string& key) const;
        double mean(const std::string& key) const;
        double stdDev(const std::string& key) const;
        std::pair<double, double> meanAndStdDev(const std::string& key) const;
        std::ostream& print(std::ostream& out) const;
        void save(const std::string& filename) const;

        void setNumberOfIterations(const long numIterations);
        void setNumberOfFunctionEvaluations(const long numEvaluations);
        void setNumberOfJacobianCalculations(const long numGradCalcs);
        void setNumberOfVariables(const size_t numVariables);
        void setNumberOfFunctions(const size_t numFunctions);

    private:
         // A struct to cross-compare KeyValuePair's and Key's in STL algorithms 
        struct KeyCompare
        {
        public:
            bool operator()(const KeyValuePair& lhs, const KeyValuePair& rhs)
            {
                return operator()(lhs.first, rhs.first);
            }

            bool operator()(const KeyValuePair& lhs, const std::string& rhs)
            {
                return operator()(lhs.first, rhs);
            }
 
            bool operator()(const std::string& lhs, const KeyValuePair& rhs)
            {
                return operator()(lhs, rhs.first);
            }

        private:
            bool operator()(const std::string& lhs, const std::string& rhs)
            {
                return (lhs < rhs);
            }
        };  //  KeyCompare
        
        struct LeftPlusRightPower2 : public std::binary_function<double, double, double>
        {
            double operator()(const double lhs, const double rhs) const
            {
                return lhs + rhs * rhs;
            }
        };  // LeftPlusRightPower2


        struct StdDevHelper : public std::binary_function<double, double, double>
        {
            StdDevHelper(const double mean):
                m_mean(mean)
            { }

            double operator()(const double lhs, const double rhs) const
            {
                return lhs + (rhs - m_mean) * (rhs - m_mean);
            }
            const double m_mean;
        };  // LeftPlusRightPower2

        
        Container::iterator lowerBound(const std::string& key);
        Container::const_iterator lowerBound(const std::string& key) const;
        
        double mean(const std::vector<double>& vec) const;
        double stdDev(const std::vector<double>& vec) const;

        Container m_tracker;
        size_t m_numberOfVariables;
        size_t m_numberOfFunctions;
        long m_numberOfIterations;
        long m_numberOfFunctionEvaluations;
        long m_numberOfJacobianCalculations;
      
    };  //  PerformanceTracker

    DECLARE_SMART_PTRS( PerformanceTracker )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const PerformanceTracker& performanceTracker)
		{
			return performanceTracker.print(out);
		}
	}
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_PERFORMANCETRACKER_H_INCLUDED