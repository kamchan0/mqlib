#include "stdafx.h"
#include "StraightLineExtendInterpolation.h"

using namespace std;

namespace FlexYCF
{
    string StraightLineExtendInterpolation::getName()
    {
        return "StraightLineExtend";
    }

    double StraightLineExtendInterpolation::evaluate(const double x) const
    {
        const_iterator upper(upperBound(x));
        
        // Flat interpolation at start before first knot-point
        if(upper == begin())
        {
            return begin()->y;
        } 
        // After last knot-point
        else if (upper == end())
        {
            // CODE SHARED WITH BiQuadraticInterpolation - could be encapsulated in base class InterpolationMethod
            if(--upper != begin())
            {
                // straight line extrapolation
                // following the slope of the last two knot-points
                const const_iterator previous(upper - 1);  
                return previous->y + (upper->y - previous->y) * (x - previous->x) / (upper->x - previous->x);
            }
            else
            {
                // just return the first value if there is only one
                // typically this will be zero in an empty curve
                return begin()->y;    
            }
        } 
        // Somewhere in-between
        else
        {
            const const_iterator lower(upper - 1);
            return lower->y + (upper->y - lower->y) * (x - lower->x) / (upper->x - lower->x);
        }

//        return 0.0;
    }

    void StraightLineExtendInterpolation::accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        GradientIterator gradientIterator(gradientBegin);
        const_iterator upper(upperBound(x));       
        
        // Before first knot-point
        if(upper == begin())
        {
            if(isUnknownKnotPoint(*upper))
            {
                *gradientIterator += multiplier;
            }
            return;        
        }
        
        // After last knot-point
        else if(upper == end())
        {
            --upper;  
        }

        const const_iterator lower(upper - 1);
        const double xDiff(upper->x - lower->x);
        
        gradientIterator += count_if(begin(), lower, isUnknownKnotPoint);

        if(isUnknownKnotPoint(*lower))
        {
            *gradientIterator += multiplier * (upper->x - x) / xDiff;
            ++gradientIterator;
        }

        if(isUnknownKnotPoint(*upper))
        {
            *gradientIterator += multiplier * (x - lower->x) / xDiff;
        }
    }

    // if x[k-1] <= x < x[k], decomposing the integral from x[0] to x 
    //  as a sum of areas of trapezia gives:
    // { (x[1]-x[0]) * (y[0]+y[1]) + ... + (x[k-1]-x[k-2]) * (y[k-1]+y[k-2])
    //      + (x-x[k-1])*(f(x)+y[k-1]) } / 2 , where f(x) is the interpolated 
    //  value computed as described above.
    // Most is just copied from StraightLine. Only the right extrapolation 
    //  behaviour changes, and is somewhat simpler.
    double StraightLineExtendInterpolation::computeIntegral(const double x) const
    {
        if(x <= begin()->x)
        {
            // flat extrapolation on the left gives
            // an easy computation
            return (x - begin()->x) * begin()->y;
        }

        const_iterator upper(upperBound(x));
        const_iterator lastIter(begin());

        double integral(0.0);

        // The integral of the interpolated curve between each couple of consecutive 
        // knots is easy to compute with the trapezium formula
        // Note: the division by 2 is done once and for all for the sum of all 
        //  trapezium areas 
        for(const_iterator iter(begin() + 1); iter != upper; ++iter)
        {
            integral += (iter->y + lastIter->y) * (iter->x - lastIter->x);
            lastIter = iter;
        }

        // When x >= x[N], add (twice) the integral of the trapezium delimited  
        //  by the points (x[N],0), (x,0), (x, f(x)) and (x[N], y[N])
        integral += (evaluate(x) + lastIter->y) * (x - lastIter->x);
        integral *= 0.5;
    
        return integral;
    }

    // Reordering the terms in the computation of the integral amd replacing f(x)
    //  by its expression in terms of y[i]'s gives (for x[k-1] <= x < x[k]):
    // { dx[1] * y[0] + (dx[1]+dx[2]) * y[1] + ... + (dx[k-2]+dx[k-1]) * y[k-2] 
    //  + [dx[k-1] + (1+(x[k]-x)/dx[k]) * (x-x[k-1])] * y[k-1] 
    //  + pow(x-x[k-1],2)/dx[k] * y[k] } / 2,
    //  where we set dx[i] := x[i] - x[i-1], so the gradient relative to the y[i]'s
    //  is immediately read from the formula
    // Essentially the same to StraightLine interpolation.
    // Only the gradient contribution from right extrapolation changes.
    void StraightLineExtendInterpolation::accumulateIntegralGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
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
        const_iterator previous(begin());
        
        for(const_iterator iter(begin()+1); iter != upper; ++iter, ++previous)
        {
            double tmp = 0.5 * (iter->x - previous->x);
            if(isUnknownKnotPoint(*(iter - 1)))
            {
                *gradientIterator += multiplier * tmp;
                ++gradientIterator;
            }
            if(isUnknownKnotPoint(*iter))
            {
                *gradientIterator += multiplier * tmp;
            }
        }

        if(upper == end())
        {
            // x is after the last knot
            // straight line extrapolation on the right adds 
            // the following terms
            const double xN((upper - 1)->x);        // last knot
            const double xN_1((upper - 2)->x);      // last but one knot
            double tmp = 1.0 / (xN - xN_1);    // 1 / (x[N] - x[N-1])                    
            
            if(isUnknownKnotPoint(*(upper - 1)))
            {
                *gradientIterator += multiplier * 0.5 * (1.0 + (x - xN_1) * tmp) * (x - xN);
            }
            if(isUnknownKnotPoint(*(upper - 2)))
            {
                --gradientIterator;
                *gradientIterator += multiplier * -0.5 * tmp * pow(x - xN, 2.0);
            }
        }
        else
        {
            if(isUnknownKnotPoint(*previous))
            {
                *gradientIterator += 0.5 * (1.0 + (upper->x - x) / (upper->x - previous->x)) * (x - previous->x);
                ++gradientIterator;
            }
            if(isUnknownKnotPoint(*upper))
            {
                *gradientIterator += 0.5 * pow(x - previous->x, 2.0) / (upper->x - previous->x);
            }
        }
    }

    InterpolationMethodPtr StraightLineExtendInterpolation::clone() const
    {
        InterpolationMethodPtr retVal(new StraightLineExtendInterpolation(*this));
        return retVal;
    }
}