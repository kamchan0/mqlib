/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "DiscountCurve.h"
#include "BaseCurve.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    DiscountCurve::DiscountCurve(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                 const string& curveDescription,
                                 const LeastSquaresResidualsPtr& leastSquaresResiduals):
        CurveFormulation(interpolationDetailsTable, curveDescription,  leastSquaresResiduals)
    {
    }

    string DiscountCurve::getName()
    {
        return "Discount";
    }

    CurveFormulationPtr DiscountCurve::createInstance(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                      const string& curveDescription,
                                                      const LeastSquaresResidualsPtr& leastSquaresResiduals)
    {
        return CurveFormulationPtr(new DiscountCurve(interpolationDetailsTable, curveDescription, leastSquaresResiduals));
    }

    double DiscountCurve::getDiscountFactor(const double flowTime) const
    {
        return m_baseCurve->evaluate(flowTime);
    }

    void DiscountCurve::accumulateDiscountFactorGradient(const double x, 
                                                         double multiplier,
                                                         GradientIterator gradientBegin,
                                                         GradientIterator gradientEnd) const
    {
        m_baseCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);
    }

	double DiscountCurve::getVariableValueFromSpineDiscountFactor(const double /* flowTime */,
															      const double discountFactor) const
	{
		return discountFactor;
	}

    /**
        @brief Pseudo copy-constructor.

        This only exists to invoke the corresponding constructor in the base class.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    DiscountCurve::DiscountCurve(DiscountCurve const& original, CloneLookup& lookup) :
        CurveFormulation(original, lookup)
    {
    }

    void DiscountCurve::onFinalize() const
    {
        // enforce a fixed (0.0, 1.0) knot point
        enforceFixedKnotPoint(0.0, 1.0);
    }

    void DiscountCurve::initializeKnotPoints() const
    {
        //	m_baseCurve->initializeKnotPoints(std::tr1::bind(&DiscountCurve::spotRateInitializer, this, _1));
		m_baseCurve->initialize([this] (double x) {return spotRateInitializer(x);});
	}

    /**
        @brief Create a clone.

        Uses a lookup to preserve directed graph relationships.

        @param lookup A lookup of previously created clones.
        @return       A clone.
    */
    ICloneLookupPtr DiscountCurve::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new DiscountCurve(*this, lookup));
    }
}