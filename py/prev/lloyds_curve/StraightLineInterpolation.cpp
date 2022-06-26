#include "stdafx.h"
#include "StraightLineInterpolation.h"

using namespace std;

namespace FlexYCF
{
    string StraightLineInterpolation::getName()
    {
        return "StraightLine";
    }

    double StraightLineInterpolation::evaluate(const double x) const
    {
        const const_iterator upper(upperBound(x));
        
        // Before first knot-point
        if(upper == begin())
        {
            // Flat extrapolation on the left
            //  (will be eventually taken care of by a LeftExtrapolationMethod)
            return begin()->y;
        } 
        // After last knot-point
        else if (upper == end())
        {
            // Flat extrapolation on the right
            //  (will be eventually taken care of by a RightExtrapolationMethod)
            const const_iterator last(end() - 1);
            return last->y;
        } 
        // Somewhere in-between
        else
        {
            const const_iterator lower(upper - 1);
            return lower->y + (upper->y - lower->y) * (x - lower->x) / (upper->x - lower->x);
        }

//        return 0.0;
    }

    void StraightLineInterpolation::accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
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
            if(isUnknownKnotPoint(*upper))
            {
                gradientIterator += count_if(begin(), upper, isUnknownKnotPoint);
                *gradientIterator += multiplier;
            }
        }
        else
        {
            const const_iterator lower(upper - 1);
            const double xDiff(upper->x - lower->x);
            
            if(isUnknownKnotPoint(*lower))
            {
                gradientIterator += count_if(begin(), lower, isUnknownKnotPoint);
                *gradientIterator += multiplier * (upper->x - x) / xDiff;
                ++gradientIterator;
            }

            if(isUnknownKnotPoint(*upper))
            {
                *gradientIterator += multiplier * (x - lower->x) / xDiff;
            }
        }
    }
    
    
    // if x[k-1] <= x < x[k], decomposing the integral from x[0] to x 
    //  as a sum of areas of trapezia gives:
    // { (x[1]-x[0]) * (y[0]+y[1]) + ... + (x[k-1]-x[k-2]) * (y[k-1]+y[k-2])
    //      + (x-x[k-1])*(f(x)+y[k-1]) } / 2 , where f(x) is the interpolated 
    //  value computed as described above.
    double StraightLineInterpolation::computeIntegral(const double x) const
    {
        if(x < begin()->x)
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

        if(lastIter + 1 == end())
        {
            // x is after the last knot, this should be the job of
            //  right extrapolation
            integral *= 0.5;    // dividing by 2 makes integral equal to the sum of areas of trapezia
            integral += lastIter->y * (x - lastIter->x);
        }
        else
        {
            // add (twice) the integral of the trapezium delimited by the points 
            //  (x[N],0), (x,0), (x, f(x)) and (x[N], y[N])
            integral += (evaluate(x) + lastIter->y) * (x - lastIter->x);
            integral *= 0.5;
        }
        
        return integral;
    }

    // Reordering the terms in the computation of the integral amd replacing f(x)
    //  by its expression in terms of y[i]'s gives (for x[k-1] <= x < x[k]):
    // { dx[1] * y[0] + (dx[1]+dx[2]) * y[1] + ... + (dx[k-2]+dx[k-1]) * y[k-2] 
    //  + [dx[k-1] + (1+(x[k]-x)/dx[k]) * (x-x[k-1])] * y[k-1] 
    //  + pow(x-x[k-1],2)/dx[k] * y[k] } / 2,
    //  where we set dx[i] := x[i] - x[i-1], so the gradient relative to the y[i]'s
    //  is immediately read from the formula
     void StraightLineInterpolation::accumulateIntegralGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
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
            if(isUnknownKnotPoint(*(upper - 1)))
            {
                *gradientIterator += multiplier * (x - previous->x);
            }
        }
        else
        {
            if(isUnknownKnotPoint(*previous))
            {
                *gradientIterator += multiplier * 0.5 * (1.0 + (upper->x - x) / (upper->x - previous->x)) * (x - previous->x);
                ++gradientIterator;
            }
            if(isUnknownKnotPoint(*upper))
            {
                *gradientIterator += multiplier * 0.5 * pow(x - previous->x, 2.0) / (upper->x - previous->x);
            }
        }
    }

    InterpolationMethodPtr StraightLineInterpolation::clone() const
    {
        InterpolationMethodPtr retVal(new StraightLineInterpolation(*this));
        return retVal;
    }

    std::ostream& StraightLineInterpolation::print(std::ostream& out) const
    {
        out << getName() << endl;
        return out;
    }
}