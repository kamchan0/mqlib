/*****************************************************************************
    
	FixedCashFlow

	Implementation of FixedCashFlow class.

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

// IDeA
#include "AssetDomain.h"
#include "FundingRepFlowKey.h"

//	FlexYCF
#include "FixedCashFlow.h"
#include "RepFlowsData.h"
#include "FlexYCFCloneLookup.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    FixedCashFlow::FixedCashFlow(const FixedCashFlowArguments& arguments):
        m_arguments(arguments),
        m_discountFactor(arguments.getDiscountFactor()),
        m_coverageTimesRate(arguments.getCoverage() * arguments.getRate())
    {
    }

    double FixedCashFlow::getValue(BaseModel const& baseModel)
    {
        return m_coverageTimesRate * m_discountFactor->getValue(baseModel);
    }

    void FixedCashFlow::accumulateGradient(BaseModel const& baseModel, 
                                           double multiplier,
                                           GradientIterator gradientBegin,
                                           GradientIterator gradientEnd)
    {
        m_discountFactor->accumulateGradient(baseModel, multiplier * m_coverageTimesRate, gradientBegin, gradientEnd);
    }

	void FixedCashFlow::accumulateGradient(BaseModel const& baseModel,
                                           double multiplier,
                                           GradientIterator gradientBegin,
                                           GradientIterator gradientEnd,
										   const CurveTypeConstPtr& curveType)
	{
		m_discountFactor->accumulateGradient(baseModel, multiplier * m_coverageTimesRate, gradientBegin, gradientEnd, curveType);
	}
	
	void FixedCashFlow::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		 m_discountFactor->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier * m_coverageTimesRate, gradientBegin, gradientEnd, spread);
	}
	
	void FixedCashFlow::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		 m_discountFactor->accumulateGradient(baseModel, multiplier * m_coverageTimesRate, gradientBegin, gradientEnd);
	}
	
	double FixedCashFlow::getRate(const BaseModel&)
	{
		return m_arguments.getRate();
	}

    void FixedCashFlow::update()
    {
        m_discountFactor->update();
    }

	void FixedCashFlow::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                     const BaseModel& model,
								     const double multiplier,
								     IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		//	add (df date, mult * cvg * rate) pair to funding rep flows
        fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomain, m_discountFactor->getArguments().getPayDate()), multiplier * m_coverageTimesRate);	
	}

    ostream& FixedCashFlow::print(ostream& out) const
    {
        m_arguments.print(out);
        out << "Fxd CF: cvg x rate = " << m_coverageTimesRate << endl;
        return out;
    }

    /**
        @brief Clone this instance.

        Use a lookup to preserve directed graph relationships.

        @param lookup A lookup of previuos created clones.
        @return       Returns a clone.
    */
    ICloneLookupPtr FixedCashFlow::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FixedCashFlow(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Use a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    FixedCashFlow::FixedCashFlow(FixedCashFlow const& original, CloneLookup& lookup) : 
        m_arguments(original.m_arguments, lookup),
        m_discountFactor(lookup.get(original.m_discountFactor)),
        m_coverageTimesRate(original.m_coverageTimesRate)
    {
    }
}

