/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_STRIPPERMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_STRIPPERMODEL_H_INCLUDED
#pragma once

//	FlexYCF
#include "SingleCurveModel.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( CurveFormulation )

    /// StripperModel strips rates out of cash and futures.
    /// 
    /// Using StripperNoCashEndDateKpp, futures and cash end
    /// dates are taken, except when the cash and first futures
    /// overlap, in which case the cash end date is ignore and
    /// the first futures start date is taken instead.
    /// In this case, no extra residual is used.
    ///
    /// Using StripperAllDatesKpp, the cash end date, the 
    /// first futures start and end dates and all other 
    /// futures end dates while minimizing the difference
    /// between two flat instantaneous forwards sharing one 
    /// knot-point.
    class StripperModel : public SingleCurveModel
    {
    public:
        explicit StripperModel(const LT::date valueDate, 
                               const string tenorDescription,
                               const LTQuant::GenericDataPtr masterTable,
                               const LTQuant::FlexYCFZeroCurvePtr parent);

        static std::string getName();
        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent); 

        /// Returns the Tenor of the model as a double
        double getTenor() const;

        // getDiscountFactor and getTenorDiscountFactor return the Tenor discount factor
        // of the model's Tenor (we could also throw an error)
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
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
															   const double discountFactor) const;

        virtual void addKnotPoint(const KnotPoint& knotPoint) const;
        virtual void setSpotRate(double time, double rate);     
        virtual void update();
        virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem); 
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem,
										   IKnotPointFunctor& onKnotPointVariableAddedToProblem);
		virtual void updateVariablesFromShifts(const LTQC::VectorDouble& variableShifts);
        virtual void finalize();

        virtual void initializeKnotPoints();
		virtual void onInitialized();

        virtual LTQuant::GenericDataPtr getSpineCurvesDetails() const;
		virtual void getSpineInternalData(SpineDataCachePtr& sdp) const;
		virtual void assignSpineInternalData(SpineDataCachePtr& sdp);

        /// Adds the unfixed knot points to the list
        virtual void getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const;
        
		virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

        /// after calib is finished destory all of the calibration
        /// instruments so they do not take up memory via the CachedInstrument component
        virtual void finishCalibration();
		std::vector<double> abscissas() const;
    protected:
        StripperModel(StripperModel const& original, CloneLookup& lookup);

    private:
        // once the knot points are placed this function is called
        // to set the number of extra residuals
        virtual void onKnotPointsPlaced();
        StripperModel(StripperModel const&); // deliberately disabled as won't clone properly

        CurveFormulationPtr m_curveFormulation; 

        double m_tenor;

    };  //  StripperModel

    DECLARE_SMART_PTRS( StripperModel )

};
#endif //__LIBRARY_PRICERS_FLEXYCF_STRIPPERMODEL_H_INCLUDED