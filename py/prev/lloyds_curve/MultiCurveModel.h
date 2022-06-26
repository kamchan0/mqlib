/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_MULTICURVEMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MULTICURVEMODEL_H_INCLUDED
#pragma once

#include "BaseModel.h"
#include "KnotPointFunctor.h"


namespace FlexYCF
{
   
    // An interface for multi-curve models
	// TO DO: be more consistent with position of 'CurveType' in interfaces
    class MultiCurveModel : public BaseModel
    {
    protected:
		MultiCurveModel() { }
        explicit MultiCurveModel(const LT::date& valueDate) :
            BaseModel(valueDate)
        {
        }

		explicit MultiCurveModel(const LTQuant::GenericData& masterTable, const LTQuant::FlexYCFZeroCurvePtr parent):
			BaseModel(masterTable, parent)
		{
		}

        MultiCurveModel(MultiCurveModel const& original, CloneLookup& lookup);

    public:
        virtual ~MultiCurveModel() = 0 { };

		/// Returns the discount factor of the spine curve with specified curve type
		virtual double getSpineDiscountFactor(const double flowTime,
											  const CurveTypeConstPtr& curveType) const;

        /// Returns the type of the Base Rate.
        virtual CurveTypeConstPtr getBaseRate() const = 0;

		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem,
										   const CurveTypeConstPtr& curveType,
										   IKnotPointFunctor& onKnotPointVariableAddedToProblem) = 0;
        
		virtual void initializeKnotPoints(const CurveTypeConstPtr& curveType,
										  const double initialSpotRate) = 0;

		// Returns the number of unknowns of the curve of the specified type
		virtual size_t getNumberOfUnknowns(const CurveTypeConstPtr& curveType) const = 0;

		// Accumulates the gradient of the discount factor for variables relative to
		// the specified curve type.
		// Note: the distance between gradientBegin and gradientEnd must be equal to the number
		// of variables on curve of the specified curve
		virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const = 0;

		// Accumulates the gradient of the Tenor discount factor for variables relative to
		// the specified curve type
		// Note: the distance between gradientBegin and gradientEnd must be equal to the number
		// of variables on curve of the specified curve
		//  *!*  The gradient is computed for the nearest LTQC::Tenor curve on the Tenor Spread       *!*
        //  *!*     Surface for now and not necessarily computed at the exact Tenor specified   *!*
        //  *!*     (this would imply stipulating an interpolation method along tenors)         *!*
        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor, 
                                                           double multiplier, 
														   GradientIterator gradientBegin, 
                                                           GradientIterator gradientEnd,
														   const CurveTypeConstPtr& curveType) const = 0;

		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
															   const double discountFactor,
															   const CurveTypeConstPtr& curveType) const = 0;

		//virtual const BaseCurveConstPtr operator[](const int label) const = 0;
        //virtual BaseCurvePtr& operator[](const int label) = 0;
        
        //virtual size_t getNumCurves() const = 0;
        // and/or getCurve(const int label) ?

        // Probably no BaseCurve container here as derived classes may want to arrange them in different ways
    
    private:
        MultiCurveModel(MultiCurveModel const&); // deliberately disabled as won't clone properly
    };

    DECLARE_SMART_PTRS( MultiCurveModel )
}
#endif //__LIBRARY_PRICERS_FLEXYCF_MULTICURVEMODEL_H_INCLUDED