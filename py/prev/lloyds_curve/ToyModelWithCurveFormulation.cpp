/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ToyModelWithCurveFormulation.h"
#include "CurveFormulation.h"
#include "DiscountCurve.h"
#include "Data\GenericData.h"
#include "LeastSquaresResiduals.h"
#include "NullDeleter.h"
#include "FlexYCFCloneLookup.h"

using namespace LTQC;

namespace FlexYCF
{

    ToyModelWithCurveFormulation::ToyModelWithCurveFormulation(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                                               const string& curveDescription)
    {
        // In practice the concrete type of the formulation will depend on a paramater in a table
        m_curveFormulation = CurveFormulationPtr(new DiscountCurve(interpolationDetailsTable,
                                                                   curveDescription,
                                                                   LeastSquaresResidualsPtr(new LeastSquaresResiduals(BaseModelPtr(this, NullDeleter()))))
                                                );
    }

    double ToyModelWithCurveFormulation::getDiscountFactor(const double flowTime) const
    {
        return m_curveFormulation->getDiscountFactor(flowTime);
    }

    double ToyModelWithCurveFormulation::getTenorDiscountFactor(const double flowTime, 
                                                                const double /* tenor */) const
    {
        return m_curveFormulation->getDiscountFactor(flowTime);
    }
    
    void ToyModelWithCurveFormulation::accumulateDiscountFactorGradient(const double flowTime, 
                                                                        double multiplier, 
                                                                        GradientIterator gradientBegin, 
                                                                        GradientIterator gradientEnd) const
    {
        m_curveFormulation->accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }

    void ToyModelWithCurveFormulation::accumulateTenorDiscountFactorGradient(const double flowTime, 
                                                                             const double /* tenor */,
                                                                             double multiplier, 
                                                                             GradientIterator gradientBegin,
                                                                             GradientIterator gradientEnd) const
    {
        m_curveFormulation->accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }

    /**
        @brief A pseudo copy constructor.

        Uses a lookup table to ensure the directed graph relationships are maintained.

        @param original The instance to be copied.
        @param lookup   A lookup of previously created clones.
    */
    ToyModelWithCurveFormulation::ToyModelWithCurveFormulation(ToyModelWithCurveFormulation const& original, CloneLookup& lookup) :
        m_curveFormulation(lookup.get(original.m_curveFormulation))
    {
    }
}