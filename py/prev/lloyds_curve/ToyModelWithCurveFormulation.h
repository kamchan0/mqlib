/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TOYMODELWITHCURVEFORMULATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TOYMODELWITHCURVEFORMULATION_H_INCLUDED
#pragma once

#include "SingleCurveModel.h"
#include "Gradient.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( CurveFormulation )

    /// A toy model to test how curve formulation fits into
    /// a model.
    class ToyModelWithCurveFormulation : public SingleCurveModel
    {
    public:
        explicit ToyModelWithCurveFormulation(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                              const string& curveDescription);

        virtual double getDiscountFactor(const double flowTime) const;
        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const;

        virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd) const;
        
        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor,
                                                           double multiplier,
                                                           GradientIterator gradientBegin,
                                                           GradientIterator gradientEnd) const;
		virtual double getVariableValueFromSpineDiscountFactor(const double /* flowTime */,
															   const double /* discountFactor */) const
		{
			return 0.0;
		}

    protected:
        ToyModelWithCurveFormulation(ToyModelWithCurveFormulation const& original, CloneLookup& lookup);

    private:
        ToyModelWithCurveFormulation(ToyModelWithCurveFormulation const&); // deliberately disabled as won't clone properly

        CurveFormulationPtr m_curveFormulation;
    };

}
#endif //__LIBRARY_PRICERS_FLEXYCF_TOYMODELWITHCURVEFORMULATION_H_INCLUDED