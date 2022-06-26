#include "stdafx.h"
#include "BiQuadraticInterpolation.h"
#include "KnotPoint.h"

using namespace std;

namespace FlexYCF
{
    BiQuadraticInterpolation::BiQuadraticInterpolation(const eExtremeInterpolationType firstQuadratic,
                                                       const eExtremeInterpolationType lastQuadratic):
        m_firstQuadratic(firstQuadratic),
        m_lastQuadratic(lastQuadratic)
    {
    }

    void  BiQuadraticInterpolation::onExtremalIteratorsSet()
    {
        const size_t nbOfPts(size());

        // Test if there are enough points     
        if(nbOfPts < 3)    
        {
            LT_LOG << "There are not enough points for a proper bi-quadratic interpolation" << endl;
            return;
        }

        const size_t nbOfPts_m1(nbOfPts - 1);
   
        //------------------------------------------------------------------------
        // Precompute some parts for the gradient
        m_alphaGrad.resize(nbOfPts);    // the 1st element  is a bit special and the last one too
        m_betaGrad.resize(nbOfPts);     //  ... same remarks
        m_gammaGrad.resize(nbOfPts);      
        
        // A few variables to help fill the gradient components initializations
        double t_im1(begin()->x);           // t[i-1]
        double t_i((begin() + 1)->x);       // t[i]
        double t_ip1;                       // t[i+1]

        double delta_i = t_i - t_im1;     // assuming there are at least 2 points....
        double delta_ip1;   // the forward difference, updated at the beginning of each iteration of the loop

        // First "Quadratic"
        // component gradients for first interval
        m_alphaGrad[0].resize(3, 0.0); 
        m_betaGrad[0].resize(3, 0.0);
        m_gammaGrad[0].resize(3, 0.0);
        
        // the x^2 coef is 0
        m_alphaGrad[0][0]   = 0.0;
        m_alphaGrad[0][1]   = 0.0;
        m_alphaGrad[0][2]   = 0.0;
        
        // point at x[-1] would not make sense
        m_betaGrad[0][0]    = 0.0;
        m_gammaGrad[0][0]   = 0.0;
            
        if(m_firstQuadratic == CONSTANT)
        { 
            m_betaGrad[0][1]    = 0.0;
            m_betaGrad[0][2]    = 0.0;
            
            m_gammaGrad[0][1]   = 1.0;
            m_gammaGrad[0][2]   = 0.0;
        }
        else if(m_firstQuadratic == LINEAR)
        {
            m_betaGrad[0][1]    = -1.0 / delta_i;
            m_betaGrad[0][2]    = 1.0 / delta_i;
            
            m_gammaGrad[0][1]   = t_i / delta_i;
            m_gammaGrad[0][2]   = -t_im1 / delta_i;
        }
        else LT_THROW_ERROR("unknown first Quadratic.");  

        /// Loop over regular quadratic
        KnotPoints::const_iterator iter_p1(begin() + 1);
        for(size_t index(1); index < nbOfPts_m1; ++index)
        {
            t_ip1 = (++iter_p1)->x;
            
            delta_ip1 = t_ip1 - t_i;

            m_alphaGrad[index].resize(3);
            m_betaGrad[index].resize(3);
            m_gammaGrad[index].resize(3);
            
            m_alphaGrad[index][0]   =  1.0 / (delta_i * (delta_i + delta_ip1));
            m_alphaGrad[index][1]   = -1.0 / (delta_i * delta_ip1);
            m_alphaGrad[index][2]   =  1.0 / (delta_ip1 * (delta_i + delta_ip1));

            m_betaGrad[index][0]    = -( t_i  + t_ip1) * m_alphaGrad[index][0];
            m_betaGrad[index][1]    = -(t_im1 + t_ip1) * m_alphaGrad[index][1];
            m_betaGrad[index][2]    = -(t_im1 +  t_i ) * m_alphaGrad[index][2];

            m_gammaGrad[index][0]   =  t_i  * t_ip1 * m_alphaGrad[index][0];
            m_gammaGrad[index][1]   = t_im1 * t_ip1 * m_alphaGrad[index][1];
            m_gammaGrad[index][2]   = t_im1 *  t_i  * m_alphaGrad[index][2];
            
            t_im1 = t_i;
            t_i = t_ip1;
            delta_i = delta_ip1;
        }

        // last quadratic
        m_alphaGrad[nbOfPts_m1].resize(3, 0.0); 
        m_betaGrad[nbOfPts_m1].resize(3, 0.0);
        m_gammaGrad[nbOfPts_m1].resize(3, 0.0);
        
        // no x^2 coef
        m_alphaGrad[nbOfPts_m1][0]  = 0.0;
        m_alphaGrad[nbOfPts_m1][1]  = 0.0;
        m_alphaGrad[nbOfPts_m1][2]  = 0.0;

        // point x[N+1] would not make sense
        m_betaGrad[nbOfPts_m1][2]   = 0.0;
        m_gammaGrad[nbOfPts_m1][2]  = 0.0;

        if(m_lastQuadratic == CONSTANT)
        {
            m_betaGrad[nbOfPts_m1][0]   = 0.0;
            m_betaGrad[nbOfPts_m1][1]   = 0.0;

            m_gammaGrad[nbOfPts_m1][0]  = 0.0;
            m_gammaGrad[nbOfPts_m1][1]  = 1.0;
        }
        else if(m_lastQuadratic == LINEAR)
        {
            m_betaGrad[nbOfPts_m1][0]   = -1.0 / delta_i;
            m_betaGrad[nbOfPts_m1][1]   = 1.0 / delta_i;

            m_gammaGrad[nbOfPts_m1][0]  =   t_i  / delta_i;
            m_gammaGrad[nbOfPts_m1][1]  = -t_im1 / delta_i;
        }
        else LT_THROW_ERROR("unknown interpolation for last quadratic");
    }

    double BiQuadraticInterpolation::evaluate(const double x) const
    {
        const_iterator upper(upperBound(x));
        
        // If before the first knot-point, extend flat on the left
        if(upper == begin())
        {
            return upper->y;
        }
        // If after the last knot-point, extend in straight line on the right
        else if(upper == end())
        {
            // CODE SHARED WITH StraightLineExtendInterpolation - could be encapsulated in base class InterpolationMethod
            if(--upper != begin()) // go back to the last knot-point on the curve, check that's not the only one 
            {
                // straight line extrapolation following the slope of the last two knot-points
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
        // Straight line interpolation if there are only 2 knot-points
        else if((upper - 1 == begin()) && (upper + 1 == end()))   
        {
            const const_iterator lower(upper - 1);
            return lower->y + (upper->y - lower->y) * (x - lower->x) / (upper->x - lower->x);
        }
        // Bi-Quadratic interpolation otherwise (or hybrid quadratic-linear mixture if there are only 3 points)
        //  just evaluate the corresponding trinomial
        else
        {
            const KnotPoints::const_iterator lower(upper - 1);
            if(lower->x == x) 
            {
                return lower->y;
            }
            else
            {
                /// Proper bi-quadratic interpolation 
                const size_t index(upper - begin());

                const double alpha_im1(computeAlpha(index-1));              //  alpha[i-1]
                const double alpha_i(computeAlpha(index));                  //  alpha[i]
                const double beta_im1(computeBeta(index-1, alpha_im1));     //  beta[i-1]
                const double beta_i(computeBeta(index, alpha_i));           //  beta[i]
                const double gamma_im1(computeGamma(index-1, alpha_im1));   //  gamma[i-1]
                const double gamma_i(computeGamma(index, alpha_i));         //  gamma[i]

                const double coef0(upper->x * gamma_im1 - lower->x * gamma_i);
                const double coef1(gamma_i - lower->x * beta_i  + upper->x * beta_im1  - gamma_im1);
                const double coef2(beta_i  - lower->x * alpha_i + upper->x * alpha_im1 - beta_im1);
                const double coef3(alpha_i -  alpha_im1);
                const double xDiffInverse(1.0 / (upper->x - lower->x));

                return xDiffInverse * ( coef0 + x * (coef1 + x * (coef2 + x * coef3))); 
            }
        }
        
    }

    void BiQuadraticInterpolation::accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        // This could be encapsulated -- see evaluate
        const_iterator upper(upperBound(x));
       
        if(upper == begin())
        {
            if(isUnknownKnotPoint(*upper))
            {
                // flat left extrapolation
                *gradientBegin += multiplier;
            }
            return;
        }
        else if(upper == end())
        {
            // gradient of a linear interpolation as in StraightLineExtendInterpolation
            --upper;  
    
            const const_iterator lower(upper - 1);
            const double xDiff(upper->x - lower->x);

            GradientIterator gradientIterator(gradientEnd);
            if(isUnknownKnotPoint(*upper))
            {
                --gradientIterator;
                *gradientIterator += multiplier * (x - lower->x) / xDiff;     
            }
            if(isUnknownKnotPoint(*lower))
            {
                --gradientIterator;
                *gradientIterator += multiplier * (upper->x - x) / xDiff;
            }
        }
        else
        {
            // retrieve the index such that:    kp[index-1].x <= x < kp[index].x
            const size_t index(upper - begin());
            const size_t index_1(index - 1);

            const const_iterator knotPointIndex_1(upper - 1);

            const double delta_i(upper->x - knotPointIndex_1->x);    
            const double x_t((x - knotPointIndex_1->x) / delta_i);
            const double one_minus_x_t(1.0 - x_t);

            // the implementation is such that this should work also for the first and last intervals, when one
            // of the qR or qL are not quadratic but only constant or linear.
            // the partial derivatives relative to index-2, index-1, index and index+1 are well-defined
            // and generally non zero 
            vector<double> qLGrad(3);  // will contain the partial derivatives of each quadratic
            vector<double> qRGrad(3);

            qLGrad[0] = m_gammaGrad[index_1][0] + x * (m_betaGrad[index_1][0] + x * m_alphaGrad[index_1][0]);
            qLGrad[1] = m_gammaGrad[index_1][1] + x * (m_betaGrad[index_1][1] + x * m_alphaGrad[index_1][1]);
            qLGrad[2] = m_gammaGrad[index_1][2] + x * (m_betaGrad[index_1][2] + x * m_alphaGrad[index_1][2]);

            qRGrad[0] = m_gammaGrad[index][0] + x * (m_betaGrad[index][0] + x * m_alphaGrad[index][0]);
            qRGrad[1] = m_gammaGrad[index][1] + x * (m_betaGrad[index][1] + x * m_alphaGrad[index][1]);
            qRGrad[2] = m_gammaGrad[index][2] + x * (m_betaGrad[index][2] + x * m_alphaGrad[index][2]);

            // Fill the 3 or 4 coordinates of the gradient that are not certainly zero
            GradientIterator gradientIterator(gradientBegin);
            if(index > 1)
            {
                gradientIterator += count_if(begin(), upper - 2, isUnknownKnotPoint);
                if(isUnknownKnotPoint(*(upper - 2)))
                {
                    *gradientIterator += multiplier * one_minus_x_t * qLGrad[0];
                    ++gradientIterator;
                }
            }
            else
            {
                // index must be 1 and so first possible thing to change is gradientBegin;
            }
            
            if(isUnknownKnotPoint(*(upper - 1)))
            {
                *gradientIterator += multiplier * (x_t * qRGrad[0] + one_minus_x_t * qLGrad[1]);
                ++gradientIterator;
            }
            if(isUnknownKnotPoint(*(upper)))
            {
                *gradientIterator += multiplier * (x_t * qRGrad[1] + one_minus_x_t * qLGrad[2]); 
                ++gradientIterator;
            }
            
            if(index < size() - 1 && isUnknownKnotPoint(*(upper + 1)))
            {
                *gradientIterator += multiplier * x_t * qRGrad[2];
            }
        }
        
    }

    InterpolationMethodPtr BiQuadraticInterpolation::clone() const
    {
        InterpolationMethodPtr retVal(new BiQuadraticInterpolation(*this));
        return retVal;
    }

    double BiQuadraticInterpolation::computeAlpha(const size_t index) const
    {
        if(index == 0 || index + 1 == size())
        {
            return 0.0;
        }

        KnotPoints::const_iterator iter_m1, iter, iter_p1;
        iter_m1 = begin() + index - 1;
        iter    = begin() + index;
        iter_p1 = begin() + index + 1;

        return ((iter_p1->y - iter->y) / (iter_p1->x - iter->x) - (iter->y - iter_m1->y) / (iter->x - iter_m1->x)) / (iter_p1->x - iter_m1->x);
    }

    double BiQuadraticInterpolation::computeBeta(const size_t index, const double alpha) const
    {
        KnotPoints::const_iterator iter_m1, iter;
        if(index == 0)
        {
            if(m_firstQuadratic == CONSTANT)
                return 0.0;
            if(m_firstQuadratic == LINEAR)
            {
                iter_m1 = begin();
                iter    = begin() + 1;
                return (iter->y - iter_m1->y) / (iter->x - iter_m1->x);
            }
            LT_THROW_ERROR("Unknown first interpolation in BiQuadratic");
        }

        if(index + 1 == size())
        {
            if(m_lastQuadratic == CONSTANT)
                return 0.0;
            if(m_lastQuadratic == LINEAR)
            {
                iter_m1 = end() - 2;
                iter    = end() - 1;
                return (iter->y - iter_m1->y) / (iter->x - iter_m1->x);
            }
            LT_THROW_ERROR("Unknown last interpolation in BiQuadratic");
        }

        iter_m1 = begin() + index - 1;
        iter    = begin() + index;

        return (iter->y - iter_m1->y) / (iter->x - iter_m1->x) - (iter_m1->x + iter->x) * alpha;
    }

    double BiQuadraticInterpolation::computeGamma(const size_t index, const double alpha) const
    {
        KnotPoints::const_iterator iter_m1, iter;

        if(index == 0)
        {
            iter_m1 = begin();
            if(m_firstQuadratic == CONSTANT)
                return iter_m1->y;
            if(m_firstQuadratic == LINEAR)
            {
                iter    = begin() + 1;
                return iter_m1->y - iter_m1->x * (iter->y - iter_m1->y) / (iter->x - iter_m1->x);
            }
            LT_THROW_ERROR("Unknown first interpolation in BiQuadratic");
        }

        if(index + 1 == size())
        {
            iter = end() - 1;
            if(m_lastQuadratic == CONSTANT)
            {
                return iter->y;
            }
            if(m_lastQuadratic == LINEAR)
            {
                iter_m1 = end() - 2;
                return iter_m1->y - iter_m1->x * (iter->y - iter_m1->y) / (iter->x - iter_m1->x);
            }
            LT_THROW_ERROR("Unknown last interpolation in BiQuadratic");
        }

        iter_m1 = begin() + index - 1;
        iter    = begin() + index;

        return iter_m1->y + iter_m1->x * (iter->x * alpha - (iter->y - iter_m1->y) / ( iter->x - iter_m1->x));
    }

    
}