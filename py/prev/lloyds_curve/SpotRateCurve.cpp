/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "SpotRateCurve.h"
#include "BaseCurve.h"

using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    SpotRateCurve::SpotRateCurve(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                 const string& curveDescription,
                                 const LeastSquaresResidualsPtr& leastSquaresResiduals):
        CurveFormulation(interpolationDetailsTable, curveDescription, leastSquaresResiduals)
    {
    }

    string SpotRateCurve::getName()
    {
        return "Spot";
    }

    CurveFormulationPtr SpotRateCurve::createInstance(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                      const string& curveDescription,
                                                      const LeastSquaresResidualsPtr& leastSquaresResiduals)
    {
        return CurveFormulationPtr(new SpotRateCurve(interpolationDetailsTable, curveDescription, leastSquaresResiduals));
    }

    double SpotRateCurve::getDiscountFactor(const double flowTime) const
    {
        return exp(-flowTime * m_baseCurve->evaluate(flowTime));
    }

    /// If K is an unknown variable of r(t), as P(t) = exp(-t * r(t)), we have:
    ///     dP(t) / dK = dP(t)/dr(t) * dr(t)/dK = -t * P(t) * dr(t)/dK
    /// so that: 
    ///                 grad(df(t)) = -t * P(t) * grad(r(t)),
    /// where the gradients are relative to the unknowns K's
    void SpotRateCurve::accumulateDiscountFactorGradient(const double flowTime,
                                                         double multiplier, 
                                                         GradientIterator gradientBegin,
                                                         GradientIterator gradientEnd) const
    {
        m_baseCurve->accumulateGradient(flowTime, multiplier * -flowTime * getDiscountFactor(flowTime), gradientBegin, gradientEnd);
    }   
	double SpotRateCurve::getVariableValueFromSpineDiscountFactor(const double flowTime,
																  const double discountFactor) const
	{
		return -log(discountFactor) / flowTime;
	}
    void SpotRateCurve::initializeKnotPoints() const
    {
        //	m_baseCurve->initializeKnotPoints(std::tr1::bind(&SpotRateCurve::initSpotRateAsFunction, this, _1));
		m_baseCurve->initialize([this] (double x) {return initSpotRateAsFunction(x);});
	}

    /**
        @brief Create a clone.

        Uses a lookup to preserve directed graph relationships.

        @param lookup A lookup of previously created clones.
        @return       A clone.
    */
    ICloneLookupPtr SpotRateCurve::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new SpotRateCurve(*this, lookup));
    }

    /**
        @brief Pseudo copy-constructor.

        This only exists to invoke the corresponding constructor in the base class.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    SpotRateCurve::SpotRateCurve(SpotRateCurve const& original, CloneLookup& lookup) :
        CurveFormulation(original, lookup)
    {
    }
}
