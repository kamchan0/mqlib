/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_LegacyToyModel_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_LegacyToyModel_H_INCLUDED

#include "BaseModel.h"
#include "ZeroCurveIF.h"

namespace FlexYCF
{
	class IKnotPointFunctor;

    // An interface for single-curve models
    class LegacyToyModel : public BaseModel
    {
    public:

		explicit LegacyToyModel(IDeA::ZeroCurveIFPtr zc);		
		
		/**
		* Get funding discount factor at given flow time
		* Delegates to underlying zero curve
		*
		* @param flowtime the time in year fraction at which the discount factor is required
		* @return the funding discount factor
		*/
        virtual double getDiscountFactor(const double flowTime) const;

		/**
		* Get index discount factor at given flow time for the given tenor
		* Delegates to underlying zero curve
		*
		* @param flowtime the time in year fraction at which the discount factor is required
		* @param tenor the tenor in year fraction of the underlying forward
		* @return the index discount factor
		*/
		virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const;

        /**
        * Return the structure factor that is exogenously computed
        * @param flowTime the time in year fraction
        * @return the structure factor
        */
        virtual double getStructureFactor(const double flowTime) const;

        virtual ~LegacyToyModel() { }
   
		/**
			@brief Create a clone.

			Uses a lookup table to ensure the directed graph relationships are maintained.

			@param lookup A lookup of previously created clones.
			@return       A clone of this instance.
		*/
		ICloneLookupPtr LegacyToyModel::cloneWithLookup(CloneLookup& lookup) const;

        /// after calib is finished destory all of the calibration
        /// instruments so they do not take up memory via the CachedInstrument component
        virtual void finishCalibration();
		// Disabled
		// ***************************************************************************************************
		virtual void update();
		virtual void finalize();
        virtual void addKnotPoint(const KnotPoint& knotPoint) const;
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem);
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem,
									   	   IKnotPointFunctor& onKnotPointVariableAddedToProblem);
		virtual void updateVariablesFromShifts(const LTQC::VectorDouble& variableShifts);
		virtual double getSpineDiscountFactor(const double flowTime,
											  const CurveTypeConstPtr& /* curveType */) const;

		virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd) const;

        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor, 
                                                           double multiplier, 
														   GradientIterator gradientBegin, 
                                                           GradientIterator gradientEnd) const;

		virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd,
													  const CurveTypeConstPtr& /* curveType */) const;

        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor, 
                                                           double multiplier, 
														   GradientIterator gradientBegin, 
                                                           GradientIterator gradientEnd,
														   const CurveTypeConstPtr& /* curveType */) const;
		
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
															   const double discountFactor) const;
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
															   const double discountFactor,
															   const CurveTypeConstPtr&) const;
        
        /// Adds the unfixed spine curves knot points to the list
        virtual void getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const;
		// END DISABLED
		// ***************************************************************************************************

		void getSpineInternalData(SpineDataCachePtr&) const {};
		void assignSpineInternalData(SpineDataCachePtr& sdp) {};

        
    protected:
		IDeA::ZeroCurveIFPtr		m_zc;

		explicit LegacyToyModel() { } 
        explicit LegacyToyModel(const LT::date& valueDate) :
            BaseModel(valueDate)
        { 
		}

		explicit LegacyToyModel(const LTQuant::GenericData& masterTable, const LTQuant::FlexYCFZeroCurvePtr parent):
			BaseModel(masterTable, parent)
		{
		}

        LegacyToyModel(LegacyToyModel const& original, CloneLookup& lookup);

    private:
        LegacyToyModel(LegacyToyModel const&); // deliberately disabled as won't clone properly
    };

    DECLARE_SMART_PTRS( LegacyToyModel )
}
#endif //__LIBRARY_PRICERS_FLEXYCF_LegacyToyModel_H_INCLUDED