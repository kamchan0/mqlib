#include "stdafx.h"
#include "PerformanceTracker.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    bool PerformanceTracker::exists(const string& key) const
    {
        Container::const_iterator lower(lowerBound(key));
        return (lower != m_tracker.end() && !KeyCompare()(key, *lower));
    }

    void PerformanceTracker::insert(const string& key)
    {
        Container::iterator lower(lowerBound(key));
        if(lower == m_tracker.end() || KeyCompare()(key, *lower))
        {
            m_tracker.insert(lower, KeyValuePair(key, vector<double>(0)));
        }
        else
        {
            LT_THROW_ERROR( "A key-value pair with the same key already exists." )
        }
    }

    vector<double>& PerformanceTracker::operator[](const string& key)
    {
        Container::iterator lower(lowerBound(key));
        if(lower != m_tracker.end() && !KeyCompare()(key, *lower))
        {
            return lower->second;
        }

        LT_THROW_ERROR( "A key-value pair with the specified key does not exist." )
    }

    const vector<double>& PerformanceTracker::operator[](const string& key) const
    {
        Container::const_iterator lower(lowerBound(key));
        if(lower != m_tracker.end() && !KeyCompare()(key, *lower))
        {
            return lower->second;
        }

        LT_THROW_ERROR( "A key-value pair with the specified key does not exist." )
    }

    vector<double>& PerformanceTracker::get(const string& key)
    {
        return operator[](key);
    }

    const vector<double>& PerformanceTracker::get(const string& key) const
    {
        return operator[](key);
    }

    double PerformanceTracker::mean(const string& key) const
    {
        return mean(operator[](key));
    }

    double PerformanceTracker::stdDev(const string& key) const
    {
        return stdDev(operator[](key));
    }

    pair<double, double> PerformanceTracker::meanAndStdDev(const string& key) const
    {
        const double mean_(mean(operator[](key)));
        return make_pair(mean_, sqrt(accumulate(operator[](key).begin(), 
                                                operator[](key).end(), 
                                                0.0, 
                                                StdDevHelper(mean_)) / static_cast<double>(operator[](key).size() - 1.0))); // non-biased std dev
    }

    ostream& PerformanceTracker::print(ostream& out) const
    {
        out.precision(5);
        out << "Steps    \t   mean \t std. dev" << endl;
        double timeSum(0.0);
        for(Container::const_iterator iter(m_tracker.begin());iter != m_tracker.end(); ++iter)
        {
            const pair<double, double> meanAndStdDev_(meanAndStdDev(iter->first));
            out << iter->first << "\t" << /*mean(iter->second)*/meanAndStdDev_.first << "\t" << /*stdDev(iter->second)*/ meanAndStdDev_.second << endl;
            timeSum += meanAndStdDev_.first;
        }
        out << "# simulations   :\t" << m_tracker.begin()->second.size() << endl;
        out << "Total avg. time :\t" << timeSum << endl;

        out << "Solver # variables:\t" << m_numberOfVariables << "\t";
        out << "Solver # functions:\t" << m_numberOfFunctions << endl;
        out << "Solver # iterations:\t" << m_numberOfIterations << endl;
        out << "Solver # function evals:\t" << m_numberOfFunctionEvaluations << "\t";
        out << "Solver # jacobian calcs:\t" << m_numberOfJacobianCalculations << endl;
        
        return out;
    }

    void PerformanceTracker::save(const string& filename) const
    {
        ofstream outputFile(filename.c_str());
        print(outputFile);
        outputFile.close();
    }

    PerformanceTracker::Container::iterator PerformanceTracker::lowerBound(const string& key)
    {
        return lower_bound(m_tracker.begin(), m_tracker.end(), key, KeyCompare());
    }

    PerformanceTracker::Container::const_iterator PerformanceTracker::lowerBound(const string& key) const
    {
        return lower_bound(m_tracker.begin(), m_tracker.end(), key, KeyCompare());
    }

    double PerformanceTracker::mean(const vector<double>& vec) const
    {
        double sum(accumulate(vec.begin(), vec.end(), 0.0));
        return sum / static_cast<double>(vec.size());
    }

    double PerformanceTracker::stdDev(const vector<double>& vec) const
    {
        double sum(accumulate(vec.begin(), vec.end(), 0.0, LeftPlusRightPower2()));
        double mean_(mean(vec));
        return sqrt(sum / static_cast<double>(vec.size()) - mean_ * mean_);
    }

    
    void PerformanceTracker::setNumberOfIterations(const long numIterations)
    {
        m_numberOfIterations = numIterations;
    }
    
    void PerformanceTracker::setNumberOfFunctionEvaluations(const long numEvaluations)
    {
        m_numberOfFunctionEvaluations = numEvaluations;
    }

    void PerformanceTracker::setNumberOfJacobianCalculations(const long numGradCalcs)
    {
        m_numberOfJacobianCalculations = numGradCalcs;
    }

    void PerformanceTracker::setNumberOfVariables(const size_t numVariables)
    {
        m_numberOfVariables = numVariables;
    }

    void PerformanceTracker::setNumberOfFunctions(const size_t numFunctions)
    {
        m_numberOfFunctions = numFunctions;
    }

    
    
}   //  FlexYCF