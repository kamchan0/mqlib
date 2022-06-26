#include "stdafx.h"
#include "CubicSplineInterpolation.h"


namespace FlexYCF
{

    // TO DO
    double CubicSplineInterpolation::evaluate(const double x) const
    {
        const_iterator upper(upperBound(x));
                                              
        
        // If before the first knot-point, extend flat
        if(upper == begin())
        {
            return upper->y;
        }
        // If after the last knot-point, extend flat
        if(upper == end())
        {
            return (--upper)->y;
        }
        // Straight line interpolation if there are less than 3 knot-points
        if((upper == begin()) || (upper == end() - 1))    // ??
        {
            const_iterator lower(upper - 1);
            return lower->y + (upper->y - lower->y) * (x - lower->x) / (upper->x - lower->x);
        }
        // else cubic interpolation : requites at least 4 spanning points (??)
        else
        {
            const const_iterator iter1(upper - 2);
            const const_iterator iter2(upper - 1);
            const const_iterator iter3(upper);
            const const_iterator iter4(upper + 1);

            // ....

            return 0.0;
        }
        
    }
 
    void CubicSplineInterpolation::accumulateGradient(const double /* x */, 
                                                      double /* multiplier */, 
                                                      GradientIterator /* gradientBegin */, 
                                                      GradientIterator /* gradientEnd */) const
    {
        LT_THROW_ERROR("Not yet implemented");
    }

    InterpolationMethodPtr CubicSplineInterpolation::clone() const
    {
        InterpolationMethodPtr retVal(new CubicSplineInterpolation(*this));
        return retVal;
    }
}