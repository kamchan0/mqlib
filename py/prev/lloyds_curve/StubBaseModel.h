/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_STUBBASEMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_STUBBASEMODEL_H_INCLUDED
#pragma once

#include "SingleCurveModel.h"
#include "Gradient.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS(StubBaseModel)

    // A test implementation of a 
    // yield curve models
    class StubBaseModel : public SingleCurveModel
    {
    public:
        explicit StubBaseModel(const double rate);
        virtual ~StubBaseModel();

        // The functions getName() and createInstance(..) are used to register this model with the ModelFactory
        static std::string getName();

         /// Creates a StubBaseModel from a table
        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr data, const LTQuant::FlexYCFZeroCurvePtr parent ); 
        
        virtual double getDiscountFactor(const double flowTime) const;
        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const;

        virtual void accumulateDiscountFactorGradient(const double /* flowTime */,
                                                      double /* multiplier */,
                                                      GradientIterator /* gradientBegin */,
                                                      GradientIterator /* gradientEnd */) const
        { }
        virtual void accumulateTenorDiscountFactorGradient(const double /* flowTime */,
                                                           const double /* LTQC::Tenor */,
                                                           double /* multiplier */,
                                                           GradientIterator /* gradientBegin */,
                                                           GradientIterator /* gradientEnd */) const
        { }
        void update() 
        { }
		void addKnotPoint(const KnotPoint&) const
		{ }
        virtual void addVariablesToProblem(const LTQuant::ProblemPtr&)
        { }
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr&,
										   IKnotPointFunctor&)
		{ }
        virtual void updateVariablesFromShifts(const LTQC::VectorDouble&)
		{ }
		virtual void finalize() 
        { }

        virtual void getSpineCurvesUnfixedKnotPoints(std::list<double>&) const
        {}


		virtual double getVariableValueFromSpineDiscountFactor(const double /* flowTime */,
															   const double /* discountFactor */) const
		{
			return 00.;
		}

        ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

		void getSpineInternalData(SpineDataCachePtr&) const {};
		void assignSpineInternalData(SpineDataCachePtr&) {};

        /// after calib is finished destory all of the calibration
        /// instruments so they do not take up memory via the CachedInstrument component
        virtual void finishCalibration();
    protected:
        StubBaseModel(StubBaseModel const& original, CloneLookup& lookup);

    private:
        StubBaseModel(StubBaseModel const&); // deliberately disabled as won't clone properly

        double m_rate;
    };
   
 
}
#endif //__LIBRARY_PRICERS_FLEXYCF_STUBBASEMODEL_H_INCLUDED