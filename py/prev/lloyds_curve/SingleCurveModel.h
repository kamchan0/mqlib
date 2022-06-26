/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEMODEL_H_INCLUDED
#pragma once

#include "BaseModel.h"


namespace FlexYCF
{
	class IKnotPointFunctor;

    // An interface for single-curve models
    class SingleCurveModel : public BaseModel
    {
    public:
        virtual ~SingleCurveModel() = 0 { }
   
        virtual void addKnotPoint(const KnotPoint& knotPoint) const = 0;
        
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem) = 0;

		virtual double getSpineDiscountFactor(const double flowTime,
											  const CurveTypeConstPtr& /* curveType */) const
		{
			// true ignoring Tenor
			return getDiscountFactor(flowTime);
		}

		virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd) const = 0;

        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor, 
                                                           double multiplier, 
														   GradientIterator gradientBegin, 
                                                           GradientIterator gradientEnd) const = 0;

		virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd,
													  const CurveTypeConstPtr& /* curveType */) const
		{
			accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
		}

        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor, 
                                                           double multiplier, 
														   GradientIterator gradientBegin, 
                                                           GradientIterator gradientEnd,
														   const CurveTypeConstPtr& /* curveType */) const
		{
			accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier, gradientBegin, gradientEnd);
		}
		
		// the concept of curve type does not make sense for single curve models
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
															   const double discountFactor) const = 0;
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
															   const double discountFactor,
															   const CurveTypeConstPtr&) const
		{
			return getVariableValueFromSpineDiscountFactor(flowTime, discountFactor);
		}

        // Design change - default implementation: by default, update the BaseCurve
        // virtual void update() const = 0;

        // Design change - default implementation: by default, add the unknowns of the BaseCurve
        // virtual void addVariablesToProblem(const LTQuant::ProblemPtr problem) const = 0;

        // virtual void finalize() const = 0;
        
    protected:
		explicit SingleCurveModel() { } 
        explicit SingleCurveModel(const LT::date& valueDate) :
            BaseModel(valueDate)
        { 
		}

		explicit SingleCurveModel(const LTQuant::GenericData& masterTable, const LTQuant::FlexYCFZeroCurvePtr parent):
			BaseModel(masterTable, parent)
		{
		}

        SingleCurveModel(SingleCurveModel const& original, CloneLookup& lookup);

    private:
        SingleCurveModel(SingleCurveModel const&); // deliberately disabled as won't clone properly
    };

    DECLARE_SMART_PTRS( SingleCurveModel )
}
#endif //__LIBRARY_PRICERS_FLEXYCF_SINGLECURVEMODEL_H_INCLUDED