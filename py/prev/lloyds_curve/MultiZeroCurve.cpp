/*****************************************************************************
    
	MultiZeroCurve

	Implementation of MultiZeroCurve


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	IDeA

//	FlexYCF
#include "MultiZeroCurve.h"
#include "GenericIRMarketData.h"
#include "BaseModel.h"
#include "FlexYCFCloneLookup.h"

//	LTQuantLib
#include "DateUtils.h"
#include "Pricers/PriceSupplier.h"
#include "Data/GenericData.h"

// QuantCore
#include "QCEnums.h"


using namespace ModuleDate;

using namespace LTQC;

namespace IDeA
{
    using namespace FlexYCF;
    using namespace LTQuant;

    MultiZeroCurve::MultiZeroCurve(LTQuant::PriceSupplier* parent, const FundingPolicyConstPtr& fp) : 
    ZeroCurve(parent, parent->getDate(), ModuleStaticData::IRIndexPropertiesPtr())
    {
        m_compositionPolicy = fp->createCompositionPolicy();
    }

	// copy constructor
	// used from the clone method 
	// mostly do a shallow copy to keep things fast
	MultiZeroCurve::MultiZeroCurve(const MultiZeroCurve& copyFrom) :
		ZeroCurve(copyFrom),
        m_compositionPolicy(copyFrom.m_compositionPolicy)
	{
	}

    /**
        @brief Create a clone of this curve.

        This clone is only deep-enough to enable the rates to be manipulated in the clone without influencing the
        original instance. A lookup is used to ensure that the directed graph relationships of the original curve
        are retained in the clone.

        Currently this is the same as shallow copy

    */
    MultiZeroCurve::MultiZeroCurve(MultiZeroCurve const& copyFrom, FlexYCF::CloneLookup& lookup) :
        ZeroCurve(copyFrom),
        m_compositionPolicy(copyFrom.m_compositionPolicy)
    {
    }

    void MultiZeroCurve::zeroCurveUpdateNotify(const string& /* currency */, 
                                                 const string& /* indexName */)
    {
        // we don't care if any other zero Curves update
    }

    void MultiZeroCurve::spotRateUpdateNotify(const string& /* main */,   
                                                const string& /* money */)
    {
        // we don't care if any spot rates change
    }
   
    double MultiZeroCurve::getDiscountFactor(LT::date flowDate) const
    {
        double timePeriod(getYearsBetweenAllowNegative(m_valueDate, flowDate));
        return getDiscountFactor2(timePeriod);
    }

    double MultiZeroCurve::getDiscountFactor2(double flowTime) const
    {
        return m_compositionPolicy->evaluate(flowTime);
    }
	
	std::vector<std::pair<std::string,double> > MultiZeroCurve::discountFactorGradient(LT::date flowDate) const
	{ 
		double time(getYearsBetweenAllowNegative(m_valueDate, flowDate));
		return m_compositionPolicy->discountFactorGradient(time);
	}
    
	double MultiZeroCurve::getTenorDiscountFactor(LT::date flowDate, double tenor) const
    {
        double timePeriod(getYearsBetweenAllowNegative(m_valueDate, flowDate));
        return getTenorDiscountFactor2(timePeriod, tenor);
    }

    double MultiZeroCurve::getTenorDiscountFactor2(double flowTime, double tenor) const
    {
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return 0.0;
    }
    double MultiZeroCurve::getStructureFactor(const LT::date& flowDate) const
    {
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return 0.0;
    }

    double MultiZeroCurve::getStructureFactor(double flowTime) const
    {
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return 0.0;
    }

    LTQuant::IRMarketDataPtr MultiZeroCurve::getMarketData() const
    {
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return LTQuant::IRMarketDataPtr();
    }

    double MultiZeroCurve::getForwardRate(LT::date flowDate1, LT::date flowDate2, DayCounterPtr const& basis) const
    {
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return 0.0;
    }

    double MultiZeroCurve::getForwardRate2(double flowTime1, double flowTime2, DayCounterPtr const& basis) const
    {
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return 0.0;
    }

    void MultiZeroCurve::getForwardRateDecomposition( const LT::date& flowDate1, const LT::date& flowDate2, const ModuleDate::DayCounterPtr& basis, double& numDf, double& denomDf, double& tenor ) const
    {
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
    }

    ZeroCurvePtr MultiZeroCurve::clone() const
    {
        CloneLookup lookup;
        ZeroCurvePtr retVal(new MultiZeroCurve(*this, lookup));
        return retVal;
    }

	FlexYCF::BaseModelPtr MultiZeroCurve::getModel() const
	{
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return FlexYCF::BaseModelPtr();
	}

	LTQuant::GenericDataPtr MultiZeroCurve::getModelParametersData() const
	{
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return LTQuant::GenericDataPtr();
	}

	LTQuant::GenericDataPtr MultiZeroCurve::getSpineCurvesDetails() const
	{
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
        return LTQuant::GenericDataPtr();
	}

	ModuleStaticData::IRIndexPropertiesPtr MultiZeroCurve::getMergedIRIndexProperties() const
	{

        LTQC_THROW(MarketException, "Can not get IR index properties from MultiZeroCurve");
        return ModuleStaticData::IRIndexPropertiesPtr();
	}
    void MultiZeroCurve::refresh()
    {
        LTQC_THROW(MarketException, "Not supported in MultiZeroCurve");
    }

};
