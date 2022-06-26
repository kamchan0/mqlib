/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FLEXYCFZEROCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FLEXYCFZEROCURVE_H_INCLUDED
#pragma once

#include "Pricers\ZeroCurve.h"
#include "math\Matrix.h"
#include "ICloneLookup.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS(BaseModel)
    FWD_DECLARE_SMART_PTRS(CalibrationInstruments)
    FWD_DECLARE_SMART_PTRS(BaseSolver)
	FWD_DECLARE_SMART_PTRS(SpineDataCache)

	enum ZeroCurveRefreshType
	{
		FromSolver,
		FromJacobian,
        FullRebuild
	};

    // Forward declarations
    class CloneLookup;
    class ICloneLookup;
}

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS(IRMarketData)
    FWD_DECLARE_SMART_PTRS(GenericIRMarketData)

	class FlexYCFZeroCurve : public ZeroCurve, public std::tr1::enable_shared_from_this<FlexYCFZeroCurve>, public FlexYCF::ICloneLookup
	{
    public:

        explicit FlexYCFZeroCurve(PriceSupplier* parent, GenericIRMarketDataPtr marketData, const string& constructionMethod, ModuleStaticData::IRIndexPropertiesPtr indexProp);
        virtual ~FlexYCFZeroCurve() {};
		void getSpineInternalData(FlexYCF::SpineDataCachePtr& sdp);
		void restoreModelSpineData(FlexYCF::SpineDataCachePtr& sdp);
        virtual double getDiscountFactor(LT::date flowDate) const;
        virtual double getDiscountFactor2(double flowTime) const;
        virtual double getTenorDiscountFactor(LT::date flowDate, double tenor) const;
        virtual double getTenorDiscountFactor2(double flowTime, double tenor) const;
        virtual double getForwardRate(LT::date flowDate1, LT::date flowDate2, ModuleDate::DayCounterPtr const& basis) const;
		virtual double getForwardRate(LT::date flowDate1, LT::date flowDate2, ModuleDate::DayCounterPtr const& basis, const FlexYCF::CurveTypeConstPtr& curveType) const;
        virtual double getForwardRate2(double flowTime1, double flowTime2, ModuleDate::DayCounterPtr const& basis) const;
        virtual double getStructureFactor(const LT::date& flowDate) const;
        virtual double getStructureFactor(double flowTime) const;
        virtual void getForwardRateDecomposition(const LT::date& flowDate1, const LT::date& flowDate2, const ModuleDate::DayCounterPtr& basis, double& numDf, double& denomDf, double& tenor) const;
		virtual void getForwardRateDecomposition( const LT::date& flowDate1, const LT::date& flowDate2, const ModuleDate::DayCounterPtr& basis, double& numDf, double& denomDf, double& tenor, const FlexYCF::CurveTypeConstPtr& curveType) const;
        virtual void zeroCurveUpdateNotify(const string& currency, const string& indexName);
        virtual void spotRateUpdateNotify(const string& main, const string& money);
        virtual LTQuant::IRMarketDataPtr getMarketData() const;
        virtual void refresh();
        virtual ZeroCurvePtr clone() const;

		// overwrite the interface to provide the appropriate properties from the curve data 
		virtual ModuleStaticData::IRIndexPropertiesPtr getMergedIRIndexProperties() const;

		// Returns the inner model
		FlexYCF::BaseModelPtr getModel() const;
       
		// Returns the model parameters used to build the inner model
		LTQuant::GenericDataPtr getModelParametersData() const;
		// Returns the details of the spine curves
		LTQuant::GenericDataPtr getSpineCurvesDetails() const;

		//	Refreshes the model using the jacobian
		void refreshFromJacobian();
		void refreshFromFullJacobian();

        LTQC::VectorDouble refreshFromNegativeInverseJacobianAndRatesShift(const std::vector<double>& dR, const LTQC::Matrix& invJacobian);
		LTQC::VectorDouble refreshFromNegativeInversePartialJacobianAndRatesShift(const std::vector<double>& dR, const LTQC::Matrix& invJacobian);
        std::vector<double> ratesShiftTimesRatesDerivative(const std::vector<double>& dR) const;
		void shockedRates(std::vector<double>& result) const;
		
        //	Set the market data
		inline void setGenericIRMarketData(const GenericIRMarketDataPtr& marketData)
		{
			m_marketData = marketData;
		}
		
        //	Set refresh required
		inline void setRefreshRequired()
		{
			m_requiresRefreshFromData = true;
		}

		//	Set the refresh type:
		void setRefreshType(const FlexYCF::ZeroCurveRefreshType refreshType);

        FlexYCF::ICloneLookupPtr cloneWithLookup(FlexYCF::CloneLookup& lookup) const;

        void getPillarPoints(std::list<double>& pillarPoints) const;

    private:
        explicit FlexYCFZeroCurve(const FlexYCFZeroCurve& copyFrom);
        FlexYCFZeroCurve(const FlexYCFZeroCurve& copyFrom, FlexYCF::CloneLookup& lookup);
        /// default constructor required by serialization
        /// should never be used by anything else
        explicit FlexYCFZeroCurve() {};

        void rebuildCurveFromData();
		void rebuildFromGenericData();
        void solveCurve();
        void lazyInit() const;
        //release the the calibration instruments
        //to minimise memory usage
        //N.B. This function will try to value full calib instruments
        //if you no longer have a valid globalComponentCache (e.g. it was on the stack in rebuildCurveData and the frame is gone)
        //this function will have undefined effects
        void finishCalibInstruments();
		void calibrationInstrumentSetValues();

		std::vector<size_t> getChildrenZeroCurves(const string& currency, const string& index) const;

        FlexYCF::BaseModelPtr				m_model;
        GenericIRMarketDataPtr				m_marketData;
        string								m_constructionMethod;
        FlexYCF::CalibrationInstrumentsPtr	m_fullInstruments;
        FlexYCF::CalibrationInstrumentsPtr	m_partialInstruments;
        FlexYCF::BaseSolverPtr				m_solver;
		bool								m_requiresRebuildFromData;
        //currently we expect to calib instrumrnts to only exist whist we are in a rebuildCurveFromData call
        //but this may change in the future hence leave a hook
        bool                                m_calibInstrumentsExist;
		FlexYCF::ZeroCurveRefreshType		m_refreshType;
		bool	                            m_requiresRefreshFromData;
        //is this a clone flexYCF zero curve, hence always clear out the calib instruments
        bool                                m_lightweightClone;
    };

    FWD_DECLARE_SMART_PTRS(FlexYCFZeroCurve)
    inline void FlexYCFZeroCurve::lazyInit() const
	{
		if (m_requiresRebuildFromData) {
			const_cast<FlexYCFZeroCurve*>(this)->rebuildCurveFromData();
			const_cast<FlexYCFZeroCurve*>(this)->m_requiresRebuildFromData = false;
			return;
		}
		if(m_requiresRefreshFromData){
			const_cast<FlexYCFZeroCurve*>(this)->refresh();
			const_cast<FlexYCFZeroCurve*>(this)->m_requiresRefreshFromData = false;
			return;
		}
	}
    
}
#endif //__LIBRARY_PRICERS_FLEXYCF_FLEXYCFZEROCURVE_H_INCLUDED