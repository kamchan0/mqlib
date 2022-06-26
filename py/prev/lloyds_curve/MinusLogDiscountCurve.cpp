/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "MinusLogDiscountCurve.h"
#include "BaseCurve.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    MinusLogDiscountCurve::MinusLogDiscountCurve(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                 const string& curveDescription,
                                                 const LeastSquaresResidualsPtr& leastSquaresResiduals):
        CurveFormulation(interpolationDetailsTable, curveDescription, leastSquaresResiduals),
        m_spotTime(0.0), m_spotRate(1.0)
    {
    }

    string MinusLogDiscountCurve::getName()
    {
        return "LogFvf";
    }

    CurveFormulationPtr MinusLogDiscountCurve::createInstance(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                              const string& curveDescription,
                                                              const LeastSquaresResidualsPtr& leastSquaresResiduals)
    {
        return CurveFormulationPtr(new MinusLogDiscountCurve(interpolationDetailsTable, curveDescription, leastSquaresResiduals));
    }

    double MinusLogDiscountCurve::getDiscountFactor(const double flowTime) const
    {
        return m_spotRate * exp(-m_baseCurve->evaluate(flowTime));
    }

    void MinusLogDiscountCurve::accumulateDiscountFactorGradient(const double x, 
                                                                 double multiplier, 
                                                                 GradientIterator gradientBegin, 
                                                                 GradientIterator gradientEnd) const
    {
        m_baseCurve->accumulateGradient(x, -getDiscountFactor(x) * multiplier, gradientBegin, gradientEnd);
    }

	double MinusLogDiscountCurve::getVariableValueFromSpineDiscountFactor(const double /* flowTime */,
																	      const double discountFactor) const
	{
		return -log(discountFactor);
	}

    /**
        @brief Create a clone.

        Uses a lookup to preserve directed graph relationships.

        @param lookup A lookup of previously created clones.
        @return       A clone.
    */
    ICloneLookupPtr MinusLogDiscountCurve::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new MinusLogDiscountCurve(*this, lookup));
    }

    /**
        @brief Pseudo copy-constructor.

        This only exists to invoke the corresponding constructor in the base class.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    MinusLogDiscountCurve::MinusLogDiscountCurve(MinusLogDiscountCurve const& original, CloneLookup& lookup) :
        CurveFormulation(original, lookup)
    { 
        m_spotTime = original.m_spotTime;
        m_spotRate = original.m_spotRate;
    }

    void MinusLogDiscountCurve::onFinalize() const
    {
        enforceFixedKnotPoint(m_spotTime, 0.0);
    }

    void MinusLogDiscountCurve::initializeKnotPoints() const
    {
        // m_baseCurve->initializeKnotPoints(bind1st(multiplies<double>(), getInitSpotRate()));
		m_baseCurve->initialize([this] (double d) {return getInitSpotRate() * d;});
    }
}