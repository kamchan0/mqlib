/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InstantaneousForwardCurve.h"
#include "BaseCurve.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    InstantaneousForwardCurve::InstantaneousForwardCurve(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                         const string& curveDescription,
                                                         const LeastSquaresResidualsPtr& leastSquaresResiduals):
        CurveFormulation(interpolationDetailsTable, curveDescription, leastSquaresResiduals)
    {
    }

    string InstantaneousForwardCurve::getName()
    {
        return "Forward";
    }

    CurveFormulationPtr InstantaneousForwardCurve::createInstance(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                                  const string& curveDescription,
                                                                  const LeastSquaresResidualsPtr& leastSquaresResiduals)
    {
        return CurveFormulationPtr(new InstantaneousForwardCurve(interpolationDetailsTable, curveDescription, leastSquaresResiduals));
    }

    double InstantaneousForwardCurve::getDiscountFactor(const double /* flowTime */) const
    {
        LT_THROW_ERROR("Not implemented in the design yet");
        // return exp(-m_baseCurve->computeIntegral(flowTime)); 
    }

    void InstantaneousForwardCurve::accumulateDiscountFactorGradient(const double /* x */, 
                                                                     double /* multiplier */, 
                                                                     GradientIterator /* gradientBegin */, 
                                                                     GradientIterator /* gradientEnd */) const
    { 
        LT_THROW_ERROR("Not implemented in the design yet");
        // m_baseCurve->accumulateIntegralGradient(x, -getDiscountFactor(x) * multiplier, gradientBegin, gradientEnd);
    }

	double InstantaneousForwardCurve::getVariableValueFromSpineDiscountFactor(const double /* flowTine */,
																	          const double /* discountFactor */) const
	{
		LT_THROW_ERROR("Not implemented yet");
	}

    void InstantaneousForwardCurve::initializeKnotPoints() const
    {
        //	m_baseCurve->initializeKnotPoints(std::tr1::bind(&InstantaneousForwardCurve::initSpotRateAsFunction, this, _1));
		m_baseCurve->initialize([this] (double x) {return initSpotRateAsFunction(x);});
	}

    /**
        @brief Pseudo copy-constructor.

        This only exists to invoke the corresponding constructor in the base class.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    InstantaneousForwardCurve::InstantaneousForwardCurve(InstantaneousForwardCurve const& original, CloneLookup& lookup) :
        CurveFormulation(original, lookup)
    {
    }

    /**
        @brief Create a clone.

        Uses a lookup to preserve directed graph relationships.

        @param lookup A lookup of previously created clones.
        @return       A clone.
    */
    ICloneLookupPtr InstantaneousForwardCurve::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new InstantaneousForwardCurve(*this, lookup));
    }
}