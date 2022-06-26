#include "stdafx.h"
#include "FlatLeftInterpolation.h"

using namespace std;

namespace FlexYCF
{
    string  FlatLeftInterpolation::getName()
    {
        return "FlatLeft";
    }

    double FlatLeftInterpolation::evaluate(const double x) const
    {   
        const_iterator lower(lowerBound(x));
        if(lower == end())
        {
            --lower;
        }
        return lower->y;
    }
 
    void FlatLeftInterpolation::accumulateGradient(const double x, 
                                                   double multiplier, 
                                                   GradientIterator gradientBegin, 
                                                   GradientIterator /* gradientEnd */) const
    {
        const_iterator lower(lowerBound(x));
        if(lower == end())
        {
            --lower;
        }
        if(isUnknownKnotPoint(*lower))
        {
            gradientBegin += count_if(begin(), lower, isUnknownKnotPoint);
            *gradientBegin += multiplier;
        }
    }

    // The integral, provided x[i-1] < x <= x[i] is between the knots, is:
    //  F(x) = y[1] * (x[1] - x[0]) + ... + y[i-1] * (x[k-1] - x[k-2]) + y[i] * (x - x[k-1])  
    double FlatLeftInterpolation::computeIntegral(const double x) const
    {
        if(x <= begin()->x)
        {
            return (x - begin()->x) * begin()->y;
        }

        const_iterator lower(lowerBound(x));
        
        double lastKnot(begin()->x);
        double integral(0.0);

        for(const_iterator iter(begin() + 1); iter != lower; ++iter)
        {
            integral += iter->y * (iter->x - lastKnot);    
            lastKnot = iter->x;
        }
        
        if(lower == end())
        {
            --lower;
        }

        // return integral + (upper == end()  ? 0.0 : upper->y * (x - lastKnot)); 
        return integral + lower->y * (x - lastKnot); 
    }
    
    // Given the formula above, the full gradient of the integral for x[i-1] < x < 0 x[i] is:
    //  (0, x[1] - x[0], ... , x[k-1] - x[k-2], x - x[k-1], 0, ... , 0)
    // Note: the first coordinate is always 0 if x after the first knot
    void FlatLeftInterpolation::accumulateIntegralGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        GradientIterator gradientIterator(gradientBegin);

        if(x <= begin()->x)
        {
            if(isUnknownKnotPoint(*begin()))
            {
                *gradientIterator += multiplier * (x - begin()->x);
            }
            return;
        }

        const_iterator lower(lowerBound(x));
        double lastKnot(begin()->x);
        if(isUnknownKnotPoint(*begin()))
        {
            ++gradientIterator;
        }

        for(const_iterator iter(begin()+1); iter != lower; ++iter)
        {
            if(isUnknownKnotPoint(*iter))
            {
                *gradientIterator += multiplier * (iter->x - lastKnot);
                ++gradientIterator;
            }
            lastKnot = iter->x;
        }

        if(lower == end())
        {
            --gradientIterator;
            --lower;
        }

        if(isUnknownKnotPoint(*lower))
        {
            *gradientIterator += multiplier * (x - lastKnot);
        }

    }

    InterpolationMethodPtr FlatLeftInterpolation::clone() const
    {
        InterpolationMethodPtr retVal(new FlatLeftInterpolation(*this));
        return retVal;
    }

}
