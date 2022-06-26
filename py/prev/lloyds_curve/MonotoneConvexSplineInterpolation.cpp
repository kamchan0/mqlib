/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "MonotoneConvexSplineInterpolation.h"
#include "KnotPoint.h"


using namespace std;

namespace FlexYCF
{
    string MonotoneConvexSplineInterpolation::getName()
    {
        return "MonotoneConvexSpline";
    }

    MonotoneConvexSplineInterpolation::MonotoneConvexSplineInterpolation(const bool enforcePositivity, 
                                                                         const double positivityCoef):
        m_enforcePositivity(enforcePositivity)
    {
        setPositivityCoef(positivityCoef);  // done here to correct the value if not appropriate.
    }

    // need to better encapsulate everything
    void MonotoneConvexSplineInterpolation::onExtremalIteratorsSet()
    {
        if(size() > 1)
        {
            const size_t kpSize(size());
            
            // Clear and resize the vectors
            m_df.clear();
            m_df.resize(kpSize);
            m_f.clear();
            m_f.resize(kpSize);
            m_K.clear();
            m_K.resize(kpSize);
            m_L.clear();
            m_L.resize(kpSize);
            m_M.clear();
            m_M.resize(kpSize);

			update();

			// Note: the g_i^L and g_i^R gradient calculations do not depend on the y's need to be done only once
            // Fill g-Left and g-Right with gradients that will speed up the computation of the gradient of the function at a value.
            //  Their 0-indexed value is not initialised as we ensure the indexes remain consistent.
            m_gL.clear();
            m_gL.resize(kpSize);
            m_gR.clear();
            m_gR.resize(kpSize);

            // double delta_i = m_knotPoints[1].x - m_knotPoints[0].x;     // assuming there are at least 2 points....
            // replaced by:
            const_iterator iter(begin());
            double delta_i = - iter->x;
            ++iter;
            delta_i += iter->x;
            
            double delta_ip1(0.);   // the forward difference, updated at the beginning of each iteration of the loop
            double delta_im1(0.);   // the difference at the last iteration, updated at the end of each iteration

            // This loop could be merged with the previous one... maybe sacrificing some clarity
            for(size_t index(1); index < kpSize; ++index)
            {
                // We need to store 3 values in all cases
                m_gL[index].resize(3);
                m_gR[index].resize(3);

                // Compute gR before gL so that it can update delta_ip1 first when it make sense                
                //gR
                if(index < kpSize - 1)
                {
                    // Update delta_ip1 here
                    // delta_ip1 = m_knotPoints[index+1].x - m_knotPoints[index].x;
                    // Replaced by (index and iter are in sync like that):
                    delta_ip1 = - iter->x;
                    ++iter;
                    delta_ip1 += iter->x; 

                    // Compute the  gradient of 'g[i]' for interval: [t[i-1], t[i][
                    m_gR[index][0] = 1.0 / (delta_i + delta_ip1);                         // relative to (i-1)-th knot-point
                    m_gR[index][1] = -1.0 / delta_ip1;                                    // rel. to i-th k-p
                    m_gR[index][2] =  delta_i / (delta_ip1 * (delta_i + delta_ip1));       // rel. to (i+1)-th k-p
                 }
                else
                {
                    // Last knot-point: No need to update delta_ip1 in this case, it wouldn't even exist anyway
                    // Compute the gradient of 'g[n]' for last interval: [t[n-1], t[n][
                    m_gR[index][0] = 0.5 * delta_i / (delta_im1 * (delta_im1 + delta_i));     // rel. to antepenultimate k-p
                    m_gR[index][1] = -0.5 / delta_im1;                                        // rel. to last but one k-p
                    m_gR[index][2] = 0.5 / (delta_im1 + delta_i);                             // rel. to last k-p
                }

                // gL
                if(index > 1)
                { 
                    // Compute the gradient of 'g[i-1]' for interval: [t[i-1], t[i][
                    m_gL[index][0] = -delta_i / ((delta_im1 + delta_i) * delta_im1);          // rel to (i-2)-th k-p
                    m_gL[index][1] = 1.0 / delta_im1;                                         // rel to (i-1)-th k-p
                    m_gL[index][2] = -1.0 / (delta_im1 + delta_i);                            // rel to i-th k-p
                }
                else
                {
                    // index == 1
                    // Compute the  gradient g[0] for the interval [t0, t1[ in H&W.
                    m_gL[index][0] = -0.5 / (delta_i + delta_ip1);                            // rel to 1st (0-indexed) k-p
                    m_gL[index][1] = 0.5 / delta_ip1;                                         // rel to 2nd (1-indexed) k-p
                    m_gL[index][2] = -0.5 * delta_i / ((delta_i + delta_ip1) * delta_ip1);    // rel to 3rd (2-indexed) k-p
                }

				// updates delta's for next iteration
                delta_im1 = delta_i;
                delta_i = delta_ip1;
            }
        }
    }

	void MonotoneConvexSplineInterpolation::update()
	{
		// 1. Compute discrete forward rates (index 0 is not initialised)
        const_iterator lastIter = begin();
        for(const_iterator iter(begin() + 1); iter != end(); ++iter)
        {
            m_df[iter - begin()] = (iter->y - lastIter->y) / (iter->x - lastIter->x);
            lastIter = iter;    // update lastIter for next iteration
        }

        // 2. Compute the f_i's (weighted avg of 2 adjacent discrete forward rates 
        // KnotPoint lastKp(m_knotPoints[0]);  // temporary variables to speed-up
        // KnotPoint thisKp(m_knotPoints[1]);  // the calculations
        // Replaced by:
        lastIter = begin();
        const_iterator thisIter(lastIter + 1);
		const_iterator nextIter;
        const size_t kpSize(size() - 1); // we only compute from 1 to the last but one element in the loop

        for(size_t kpCnt(1); kpCnt < kpSize; ++kpCnt)
        {
            // const KnotPoint nextKp(m_knotPoints[kpCnt+1]);
            //  m_f[kpCnt] = ((thisKp.x - lastKp.x) * m_df[kpCnt+1] + (nextKp.x - thisKp.x) * m_df[kpCnt]) / (nextKp.x - lastKp.x); 
    
            //  Replaced by:
            nextIter = thisIter + 1;
             m_f[kpCnt] = ((thisIter->x - lastIter->x) * m_df[kpCnt+1] + (nextIter->x - thisIter->x) * m_df[kpCnt]) / (nextIter->x - lastIter->x); 
            
            // Initialize the K, L, M we can on this interval
            if(kpCnt > 1)
            {
                computeKLM(kpCnt);
            }

			// LOG DEBUG INFO
			LT_LOG << thisIter->x << "\t" << kpCnt << "\t" << m_df[kpCnt] << "\t" << m_f[kpCnt] << "\t" 
				<< ((m_df[kpCnt] - m_f[kpCnt]) * (m_df[kpCnt] - m_f[kpCnt-1]) < 0? 1 : 0) << std::endl;


            // shift knot-points for next iteration
            // lastKp = thisKp;
            // thisKp = nextKp;
            // replaced by:
            lastIter = thisIter;
            thisIter = nextIter;
        }

        // Compute f[0] and f[n] ...
        m_f[0]      = m_df[1]       - 0.5 * (m_f[1] - m_df[1]);                 // chosen so that f'(0) = 0, can be replaced by overnight rate 
        m_f[kpSize] = m_df[kpSize]  - 0.5 * (m_f[kpSize-1] - m_df[kpSize]);     // chosen so that f'(t_n) = 0. somewhat arbitrary too

		// LOG DEBUG INFO for last point / interval
		LT_LOG << (begin() + kpSize)->x << "\t" << kpSize << "\t" << m_df[kpSize] << "\t" << m_f[kpSize] << "\t" 
			<< ((m_df[kpSize] - m_f[kpSize]) * (m_df[kpSize] - m_f[kpSize-1]) < 0? 1 : 0) << std::endl;
		// LOG DEBUG INFO of first point
		LT_LOG << begin()->x << "\t" << 0 << "\t" << m_df[0] << "\t" << m_f[0] << std::endl;
		
        // .. and compute K, L, M at 1 and n (now we can)
        // Note: K, L, M not computed at 0
        computeKLM(1);
        computeKLM(kpSize); // index of the last elements as it has decremented
        
         // This is to enforce positivity of the interpolant (see Hagan & West pp 25, 26)
        if(m_enforcePositivity)
        {
            enforceInterpolantPositivity();
        }
	}

    void MonotoneConvexSplineInterpolation::computeKLM(const size_t idx)
    {
        m_K[idx] = m_f[idx - 1];
        m_L[idx] = 2.0 * (3.0 * m_df[idx] - 2.0 * m_K[idx] - m_f[idx]);
        m_M[idx] = 3.0 * (m_K[idx] + m_f[idx] - 2.0 * m_df[idx]);
    }


    
    // This doesn't work with the gradient at the moment at the truncation function is not differentiable at certain points.
    void MonotoneConvexSplineInterpolation::enforceInterpolantPositivity()
    {
        double g_i;
        double g_iMinus1;
        
        double f_loc_min; // here for debug, can compute directly the local min

        // The first interval is special - specific shifts for the first and last f_i'1 are encapsulated in the shift_f function

        // First Interval
        g_iMinus1   = m_f[0] - m_df[1];     // g_0
        g_i         = m_f[1] - m_df[1];     // g_1
        f_loc_min   = f_local_minimum(1);     

        bool f0_modified = false;   // to keep track if we change m_f[0]

        // Check the g_i's are in the quadrant that can cause problem for this interval
        if(g_iMinus1 > 0.0 && g_i > 0.0 /*&& f_loc_min < 0.0*/)
        {
            if(!liesWithinRange(m_f[0], 1))
            {
                shift_f(0);
                f0_modified = true;
            }

            if(!liesWithinRange(m_f[1], 1) && !f0_modified)
            {
                shift_f(1);
            }
        }
        
        // we go through all intervals from the second one to the last one 
        for(size_t index(2); index < m_f.size(); ++index)
        {
            g_iMinus1   = m_f[index-1]  - m_df[index];              
            g_i         = m_f[index]    - m_df[index];
            f_loc_min = f_local_minimum(index);  

             // Check the g[i-1] and g[i] are in the quadrant that can cause problem for this interval
            if(g_iMinus1 > 0.0 && g_i > 0.0/* && f_loc_min < 0.0*/)
            {
                // "if the application sets f0 then we cannot apply the first shift"
                if((index > 2 || !f0_modified) && !liesWithinRange(m_f[index-1], index))
                {
                    shift_f(2);
                }

                if(!liesWithinRange(m_f[index], index))
                {
                    shift_f(index);
                }
            }
        }

         // m_f[lastIndex] = min(max(0.0, m_f[lastIndex]), positivityCoef * m_df[lastIndex]);
    }
    
    const bool MonotoneConvexSplineInterpolation::liesWithinRange(const double x, const size_t index) const
    {
        return (0.0 < x && x < m_positivityCoef * m_df[index]);
    }

    // see Hagan&West p. 26
    void MonotoneConvexSplineInterpolation::shift_f(const size_t index)
    {
        if(index == 0)
        {
            // first element        
            m_f[0] = min(max(0.0, m_f[0]), m_positivityCoef * m_df[1]);    // m_df[1] and not m_df[0] which is not even well-defined
        }
        else 
        {
            const size_t lastIndex(m_df.size() - 1);
            if(index < lastIndex)
            {
                // second to last but one elements
                m_f[index] = min(max(0.0, m_f[index]), m_positivityCoef * min(m_df[index], m_df[index+1]));
            }
            else
            {
                // last element
                m_f[lastIndex] = min(max(0.0, m_f[lastIndex]), m_positivityCoef * m_df[lastIndex]);
            }
        }
    }

    const double MonotoneConvexSplineInterpolation::f_local_minimum(const size_t index) const
    {
        //if index < 1 || index >= m_f.size() throw ...
        return (m_f[index-1] - m_df[index]) * (m_f[index] - m_df[index]) / (2.0 * m_df[index] - m_f[index-1] - m_f[index]) + m_df[index];
    }


    double MonotoneConvexSplineInterpolation::evaluate(const double x) const
    {
        const_iterator upper(upperBound(x));
        
        // If before the first knot-point, extend flat
        if(upper == begin())
        {
            return upper->y;
        }
        // If after the last knot-point, extend flat
        else if(upper == end())
        {
            return (--upper)->y;
        }
        // if less than 4 points, straight line
        else if(size() < 4) 
        {
            const const_iterator lower(upper - 1);
            return lower->y + (upper->y - lower->y) * (x - lower->x) / (upper->x - lower->x);
        }
        else
        {
            const size_t index(upper - begin());
        
            // these 2 lines were replaced by:
            const const_iterator previous(upper - 1);
            const double delta_t(upper->x - previous->x);
            const double x_t((x - previous->x) / delta_t);
        
            // coefficient of the polynomial in x(t) in the remainder integral from t[i-1] to t (x in the function)
            const double coef1(m_f[index - 1] - m_df[index]);       // could be precomputed - this is also g[i-1]
            // const double g_i(m_f[index] - m_df[index]);             // g[index] just for debug... in particular we g[i]>0 and g[i-1]>0, we might have negative forwards
            const double coef3(coef1 + m_f[index] - m_df[index]);   // could be precomputed
            const double coef2(coef1 + coef3);

            const double remainderIntegral(delta_t * x_t * (coef1 + x_t * (-coef2 + x_t * coef3)));

            //return m_knotPoints[index].y * x_t + m_knotPoints[index - 1].y * (1.0 - x_t) + remainderIntegral;
            // Replaced by:
            return upper->y * x_t + previous->y * (1.0 - x_t) + remainderIntegral;
        }
    }
 
    void MonotoneConvexSplineInterpolation::accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        const const_iterator upper(upperBound(x));
       
        if(upper == begin()) 
        {
            if(isUnknownKnotPoint(*upper))
            {
                // flat left extrapolation
                *gradientBegin += multiplier;
            }
        }
        else if(upper == end())
        {
            GradientIterator gradientIterator(gradientEnd - 1);
            if(isUnknownKnotPoint(*(upper - 1)))
            {
                *gradientIterator += multiplier;     
            }
        }
        else if(size() < 4)
        {
            // gradient of a linear interpolation as in StraightLineExtendInterpolation
            const const_iterator lower(upper - 1);
            const double xDiff(upper->x - lower->x);

            GradientIterator gradientIterator(gradientBegin);
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
        else
        {
            GradientIterator gradientIterator(gradientBegin);

            // retrieve the index such that:    kp[index-1].x <= x < kp[index].x
            size_t index(upper - begin());
 
            // const double delta_i(m_knotPoints[index].x - m_knotPoints[index - 1].x);    
            // const double x_t((x - m_knotPoints[index-1].x) / delta_i);
            // these two lines were replaced by:
            const const_iterator previous(upper - 1);
            const double delta_i(upper->x - previous->x);    
            const double x_t((x - previous->x) / delta_i);

            const double phi_t(delta_i * x_t * (1 + x_t * (-2.0 + x_t)));   // the polynomial in front of g(index-1)    (inc. delta_i)
            const double psi_t(delta_i * x_t * x_t * (x_t - 1.0));          // the polynomial in front of g(index)      (inc. delta_i)
            
            // special cases : index == 1 (1st interval) and index == m_knotPoints.size() - 1 (last interval)
            if(index == 1)
            {
                if(isUnknownKnotPoint(*begin()))
                {
                    *gradientIterator += multiplier * (1 - x_t + phi_t * m_gL[1][0] + psi_t * m_gR[1][0]);    // d / dR0;
                    ++gradientIterator;
                }
                if(isUnknownKnotPoint(*(begin() + 1)))
                {
                    *gradientIterator += multiplier * (x_t   + phi_t * m_gL[1][1] + psi_t * m_gR[1][1]);    // d / dR1;
                    ++gradientIterator;
                }
                if(isUnknownKnotPoint(*(begin() + 2)))
                {
                    *gradientIterator += multiplier * (phi_t * m_gL[1][2] + psi_t * m_gR[1][2]);    // d / dR2;
                    ++gradientIterator;
                }
            }
            else if(index == size() - 1)
            {
                gradientIterator = gradientEnd - 1;
                const size_t lastIndex(size() - 1);

                if(isUnknownKnotPoint(*(end() - 1)))
                {
                    *gradientIterator += multiplier * (x_t   + phi_t * m_gL[lastIndex][2] + psi_t * m_gR[lastIndex][2]);
                }
                if(isUnknownKnotPoint(*(end() - 2)))
                {
					--gradientIterator;
                    *gradientIterator += multiplier * (1 - x_t + phi_t * m_gL[lastIndex][1] + psi_t * m_gR[lastIndex][1]);
                }
                if(isUnknownKnotPoint(*(end() - 3)))
                {
					--gradientIterator;
                    *gradientIterator += multiplier * (phi_t * m_gL[lastIndex][0] + psi_t * m_gR[lastIndex][0]);
                }
            }
            else
            {
                gradientIterator += count_if(begin(), upper - 2, isUnknownKnotPoint);

                if(isUnknownKnotPoint(*(upper - 2)))
                {
                    *gradientIterator += multiplier * (phi_t * m_gL[index][0]);
                    ++gradientIterator;
                }
                if(isUnknownKnotPoint(*(upper - 1)))
                {
                    *gradientIterator += multiplier * (1 - x_t + phi_t * m_gL[index][1] + psi_t * m_gR[index][0]);
                    ++gradientIterator;
                }
                if(isUnknownKnotPoint(*upper))
                {
                    *gradientIterator += multiplier * (x_t + phi_t * m_gL[index][2] + psi_t * m_gR[index][1]);
                    ++gradientIterator;
                }
                if(isUnknownKnotPoint(*(upper + 1)))
                {
                    *gradientIterator += multiplier * (psi_t * m_gR[index][2]);
                    ++gradientIterator;
                }
            }   // case 1 < index < m_knotPoints.size() - 1
        }
    }   // computeFullGradient

    const bool MonotoneConvexSplineInterpolation::getEnforcePositivity() const
    {
        return m_enforcePositivity;
    }
    
    void MonotoneConvexSplineInterpolation::setEnforcePositivity(const bool enforcePositivity)
    {
        m_enforcePositivity = enforcePositivity;
    }

    const double MonotoneConvexSplineInterpolation::getPositivityCoef() const
    {
        return m_positivityCoef;
    }

    void MonotoneConvexSplineInterpolation::setPositivityCoef(const double positivityCoef)
    {
        m_positivityCoef = positivityCoef;
        // Not sure about the exact coef bounds that enforce positivity
        if(m_positivityCoef < 1.0 || m_positivityCoef > 3.0)
        {
            m_positivityCoef = 2.0;
        }
    }

    InterpolationMethodPtr MonotoneConvexSplineInterpolation::clone() const
    {
        InterpolationMethodPtr retVal(new MonotoneConvexSplineInterpolation(*this));
        return retVal;
    }

    std::ostream& MonotoneConvexSplineInterpolation::print(std::ostream& out) const
    {
        out << getName() << endl;
        return out;
    }
}