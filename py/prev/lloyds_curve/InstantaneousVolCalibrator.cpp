#include "stdafx.h"
#include "InstantaneousVolCalibrator.h"
#include "Models/InstantaneousVol.h"
#include "Maths/LeastSquaresProblem.h"
#include "Maths/LevenbergMarquardt.h"

using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
    double InstantaneousVolCalibrator::calibrate(const LTQuant::InstantaneousVolPtr& instantaneousVol, 
                                                 const ImpliedVolQuotes& impliedCapletVolQuotes,
                                                 const double epsilon,
                                                 const long maxFunctionEvals,
                                                 const double ftol,
                                                 const double xtol,
                                                 const double gtol)
    {
        m_instantaneousVol = instantaneousVol;
        m_impliedVolQuotes = &impliedCapletVolQuotes;
        
        // Fits the instantaneous volatility to the implied caplet volatilities
        LTQuant::LeastSquaresProblem leastSquaresProblem(m_impliedVolQuotes->size(),
														 [this] (size_t index) {return impliedCapletVolDiff(index);},
														 [this] () {return update();});
        
        m_instantaneousVol->addToProblem(leastSquaresProblem);
        leastSquaresProblem.setToDefaults();

        LevenbergMarquardt levMar(epsilon, maxFunctionEvals, ftol, xtol, gtol);
        
        return levMar.minimize(leastSquaresProblem);
     }

    // Returns the difference between the average volatility of the "index-th" forward rate
    // up to its time to expiry minus the caplet vol
    double InstantaneousVolCalibrator::impliedCapletVolDiff(const size_t index)
    {
        m_expiryTmp     = (*m_impliedVolQuotes)[index].getTimeToExpiry();
        m_capletVolTmp  = (*m_impliedVolQuotes)[index].getVolatility();

        return (sqrt(m_instantaneousVol->getIntegralSigmaSigma(0.0, m_expiryTmp, m_expiryTmp, m_expiryTmp) / m_expiryTmp)
            - m_capletVolTmp);
    }

    void InstantaneousVolCalibrator::update() const
    {
        m_instantaneousVol->onUpdate();
    }
    
}