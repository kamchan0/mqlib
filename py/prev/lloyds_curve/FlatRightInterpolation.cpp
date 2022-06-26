#include "stdafx.h"
#include "FlatRightInterpolation.h"

using namespace std;

namespace FlexYCF
{
    string FlatRightInterpolation::getName()
    {
        return "FlatRight";
    }

    double FlatRightInterpolation::evaluate(const double x) const
    { 
        const_iterator upper(upperBound(x));
        if(upper != begin())
        {
            --upper;
        }
        return upper->y;
    }
 
    void FlatRightInterpolation::accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        const_iterator upper(upperBound(x));
        if(upper != begin())
        {
            --upper;
        }
        if(isUnknownKnotPoint(*upper))
        {
            gradientBegin += count_if(begin(), upper, isUnknownKnotPoint);
            *gradientBegin += multiplier;
        }
    }


    // The integral, provided x[i-1] <= x < x[i] is between the knots, is:
    //  F(x) = y[0] * (x[1] - x[0]) + ... + y[i-2] * (x[k-1] - x[k-2]) + y[i-1] * (x - x[k-1])  
    double FlatRightInterpolation::computeIntegral(const double x) const
    {
        if(x < begin()->x)
        {
            // This should be the job of LeftExtrapolation:
            // if x0 is the first knot and the computation of integral from
            // x0 to x is required, it should return - the integral from x to x0
            // as computed by left extrapolation
            return (x - begin()->x) * begin()->y;   
        }

        const_iterator upper(upperBound(x));
        const_iterator lastIter(begin());

        double integral(0.0);
        
        for(const_iterator iter(begin() + 1); iter != upper; ++iter)
        {
            integral += lastIter->y * (iter->x - lastIter->x);
            lastIter = iter;
        }

        return integral + lastIter->y * (x - lastIter->x);
        
        // to avoid computing any integral outside knots when right extrapolation is available, should be:
        //  return integral + (upper == end()? 0.0 : lastIter->y * (x - lastIter->x));
    }

    // Given the formula above, the full gradient of the integral for x[i-1] <= x < x[i] is:
    //  (x[1] - x[0], ... , x[k-1] - x[k-2], x - x[k-1], 0, ... , 0)
    // Note: the last coordinate is always zero if x i before the last knot
    void FlatRightInterpolation::accumulateIntegralGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
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

        const_iterator upper(upperBound(x));
        const_iterator lastIter(begin());

        for(const_iterator iter(begin() + 1); iter != upper; ++iter)
        {
            if(isUnknownKnotPoint(*lastIter))
            {
                *gradientIterator += multiplier * (iter->x - lastIter->x);
                ++gradientIterator;
            }
            ++lastIter;
        }

        if(isUnknownKnotPoint(*lastIter))
        {
            *gradientIterator += multiplier * (x - lastIter->x);
        }
    }


    InterpolationMethodPtr FlatRightInterpolation::clone() const
    {
        InterpolationMethodPtr retVal(new FlatRightInterpolation(*this));
        return retVal;
    }
}
