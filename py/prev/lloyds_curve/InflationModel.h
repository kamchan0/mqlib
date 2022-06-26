/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INFLATIONMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INFLATIONMODEL_H_INCLUDED
#pragma once

//	FlexYCF
#include "SingleCurveModel.h"
#include "Gradient.h"

//	LTQC
#include "QCException.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( ICurve )
	FWD_DECLARE_SMART_PTRS( TransformFunction )

    /// Basic inflation model
    class InflationModel : public SingleCurveModel
    {
    public:
        explicit InflationModel(const LTQuant::GenericDataPtr& masterTable, const LTQuant::FlexYCFZeroCurvePtr parent);

		// Returns the projected value of the inflation index
        virtual double getDiscountFactor(const double flowTime) const;
        
		// Returns the raw projected value of the inflation index, not seasonally or structure adjusted
		virtual double getTenorDiscountFactor(const double flowTime, const double) const;

		// Returns the seasonal and structural adjustments factor
		double getAdjustmentFactor(const double flowTime) const;

        /**
        * Return the seasonality factor that is exogenously computed
        * @param flowTime the time in year fraction
        * @return the seasonality factor
        */
        virtual double getStructureFactor(const double flowTime) const
        {
            return getSeasonality(flowTime);
        }

		// Returns the name for the InflationModel type
        static std::string getName();

        /// Creates a InflationModel from a table
        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr& data, const LTQuant::FlexYCFZeroCurvePtr parent); 

        // Computes the DiscountFactor sensitivities with respect to the knot-points' variables
        //  at a certain flow-time
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

        // Change back: implementations are back here as 
        //  a SingleCurveModel can be made of a BaseCurve or a CurveFormulation    
        virtual void addKnotPoint(const KnotPoint& knotPoint) const;
        // virtual void addInitialSpotRate(const double flowTime,
        //                                const double spotRate) const;
        virtual void update();
        virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem);
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem,
										   IKnotPointFunctor& onKnotPointVariableAddedToProblem);
		virtual void updateVariablesFromShifts(const LTQC::VectorDouble&)
		{
			LTQC_THROW( LTQC::ModelQCException, "updateVariablesFromShift is not implemented for InflationModel" );
		}
        virtual void finalize();

		// transforms a value using the transform function
		// used for analytical initialization of the inflation curve
		double inflationIndexToVariable(const double x) const;

		virtual LTQuant::GenericDataPtr getSpineCurvesDetails() const;

		void getSpineInternalData(SpineDataCachePtr&) const ;
		void assignSpineInternalData(SpineDataCachePtr& sdp);

        /// Adds the unfixed spine curves knot points to the list
        virtual void getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const;
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

        /// after calib is finished destory all of the calibration
        /// instruments so they do not take up memory via the CachedInstrument component
        virtual void finishCalibration();
    protected:
        InflationModel(InflationModel const& original, CloneLookup& lookup);
	
    private:
        ICurvePtr createSeasonality(const LTQuant::GenericDataPtr& data);
        ICurvePtr createBaseCurve(const LTQuant::GenericDataPtr& data);
        double getTheoreticalYears(const double time) const;
        double getSeasonality(const double flowTime) const;
        InflationModel(InflationModel const&); // deliberately disabled as won't clone properly

        ICurvePtr				m_seasonality;
        ICurvePtr				m_baseCurve;   // here for now
		TransformFunctionPtr	m_transformFunction;
    };

    DECLARE_SMART_PTRS( InflationModel )
}
#endif //__LIBRARY_PRICERS_FLEXYCF_INFLATIONMODEL_H_INCLUDED