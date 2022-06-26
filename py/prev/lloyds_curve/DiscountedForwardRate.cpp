/*****************************************************************************
    
	DiscountedForwardRate

	Implementation of the DiscounetdForwardRate class.
    
	@Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

// IDeA
#include "AssetDomain.h"

//	FlexYCF
#include "DiscountedForwardRate.h"
#include "DiscountedForwardRateArguments.h"
#include "GenericInstrumentComponent.h"
#include "TenorDiscountFactor.h"
#include "BaseModel.h"
#include "ForwardRateArguments.h"
#include "ForwardRate.h"
#include "FlexYcfUtils.h"
#include "RepFlowsData.h"
#include "FlexYCFCloneLookup.h"

using namespace std;
using namespace LTQC;
using namespace IDeA;

namespace FlexYCF
{
    double DiscountedForwardRate::getValue(BaseModel const& baseModel)
    {
        return (m_coverage * m_forwardRate->getValue(baseModel) * m_discountFactor->getValue(baseModel));            
    }

    // As the discounted forward rate (DFR), is cvg * forward rate * discount factor, its gradient can be computed as:
    //  grad(DFR) = cvg * [ FwdRate * grad(DF) + DF * grad(FwdRate) ]
    void DiscountedForwardRate::accumulateGradient(BaseModel const& baseModel, 
												   double multiplier, 
												   GradientIterator gradientBegin, 
												   GradientIterator gradientEnd)
    {
        // compute the right side inside the brackets of the above formula
        m_forwardRate->accumulateGradient(baseModel, multiplier * m_coverage * m_discountFactor->getValue(baseModel), gradientBegin, gradientEnd);

        // compute the left side inside the brackets of the above formula
        m_discountFactor->accumulateGradient(baseModel, multiplier * m_coverage * m_forwardRate->getValue(baseModel), gradientBegin, gradientEnd);
    }

	 void DiscountedForwardRate::accumulateGradient(BaseModel const& baseModel, 
													double multiplier,
													GradientIterator gradientBegin,
													GradientIterator gradientEnd,
													const CurveTypeConstPtr& curveType)
	 {
		// compute the right side inside the brackets of the above formula
		m_forwardRate->accumulateGradient(baseModel, multiplier * m_coverage * m_discountFactor->getValue(baseModel), gradientBegin, gradientEnd, curveType);

        // compute the left side inside the brackets of the above formula
        m_discountFactor->accumulateGradient(baseModel, multiplier * m_coverage * m_forwardRate->getValue(baseModel), gradientBegin, gradientEnd, curveType);
	 }
	 
	 void DiscountedForwardRate::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread) 
	 {
		m_forwardRate->accumulateGradient(baseModel, multiplier * m_coverage * m_discountFactor->getValue(dfModel), gradientBegin, gradientEnd);
		if( spread )
		{
			double  spreadDf = dfModel.getSpreadDiscountFactor(m_discountFactor->getArguments().getFlowTime());
			m_discountFactor->accumulateGradient(baseModel, multiplier * m_coverage * m_forwardRate->getValue(baseModel) * spreadDf,gradientBegin, gradientEnd);
		}
	 }
	 
	 void DiscountedForwardRate::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread) 
	{
		m_forwardRate->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier * m_coverage * m_discountFactor->getValue(baseModel), gradientBegin, gradientEnd,spread);
		m_discountFactor->accumulateGradient(baseModel, multiplier * m_coverage * m_forwardRate->getValue(dfModel), gradientBegin, gradientEnd);
	}

	double DiscountedForwardRate::getRate(const BaseModel& model)
	{
		return m_forwardRate->getValue(model);
	}

    void DiscountedForwardRate::update()
    {
        m_arguments.getForwardRate()->update();
        m_arguments.getDiscountFactor()->update();    
    }

	void DiscountedForwardRate::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                             const BaseModel& model,
											 const double multiplier,
											 IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		//	add (df date, mult * cvg * fwd rate) pair
		fundingRepFlows.addRepFlow(IDeA::Funding::Key(model.primaryAssetDomainFunding(), m_discountFactor->getArguments().getPayDate()), multiplier * m_coverage * m_forwardRate->getValue(model));
	}

	void DiscountedForwardRate::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                             const BaseModel& model,
											 const double multiplier,
											 IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		//	delegate to the forward rate, multiplying by cvg * df
		m_forwardRate->fillRepFlows(model.primaryAssetDomainIndex(),model, multiplier * m_coverage * m_discountFactor->getValue(model), indexRepFlows);
	}

    ostream& DiscountedForwardRate::print(ostream& out) const
    {
        out << m_arguments << endl;
        return out;
    }

    /**
        @brief Clone this instance.

        Use a lookup to preserve directed graph relationships.

        @param lookup A lookup of previuos created clones.
        @return       Returns a clone.
    */
    ICloneLookupPtr DiscountedForwardRate::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new DiscountedForwardRate(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Use a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    DiscountedForwardRate::DiscountedForwardRate(DiscountedForwardRate const& original, CloneLookup& lookup) : 
        m_arguments(original.m_arguments, lookup),
        m_forwardRate(lookup.get(original.m_forwardRate)),
        m_discountFactor(lookup.get(original.m_discountFactor)),
        m_coverage(original.m_coverage)
    {
    }
}   //  FlexYCF


namespace FlexYCF
{
   
    double DiscountedForwardRateNotionalExchange::getValue(BaseModel const& domModel, BaseModel const& forModel, double fx)
    {
        double notional = getNotional(domModel, forModel, fx);
        return (( 1.0 + m_coverage * m_forwardRate->getValue(domModel) ) * m_discountFactor->getValue(domModel) - m_startDateDiscountFactor->getValue(domModel)) * notional;            
    }
	 
	double DiscountedForwardRateNotionalExchange::getNotional(BaseModel const& domModel, BaseModel const& forModel, double fx)
    {
        return 1.0/fx * m_foreignFixingDateDiscountFactor->getValue(forModel)/m_domesticFixingDateDiscountFactor->getValue(domModel) * m_domesticSpotFxDateDiscountFactor->getValue(domModel)/m_foreignSpotFxDateDiscountFactor->getValue(forModel);
    }
    
    void DiscountedForwardRateNotionalExchange::accumulateGradient(BaseModel const& domModel, BaseModel const& forModel, double fx,
												   double multiplier, 
												   GradientIterator gradientBegin, 
												   GradientIterator gradientEnd)
    { 
        m_foreignFixingDateDiscountFactor->accumulateGradient(forModel, multiplier * getValue(domModel,forModel,fx)/m_foreignFixingDateDiscountFactor->getValue(forModel), gradientBegin, gradientEnd);
		m_foreignSpotFxDateDiscountFactor->accumulateGradient(forModel, - multiplier * getValue(domModel,forModel,fx)/m_foreignSpotFxDateDiscountFactor->getValue(forModel), gradientBegin, gradientEnd);
    }

	 void DiscountedForwardRateNotionalExchange::accumulateGradient(BaseModel const& domModel, BaseModel const& forModel, double fx,
													double multiplier,
													GradientIterator gradientBegin,
													GradientIterator gradientEnd,
													const CurveTypeConstPtr& curveType)
	 {
		 m_foreignFixingDateDiscountFactor->accumulateGradient(forModel, multiplier * getValue(domModel,forModel,fx)/m_foreignFixingDateDiscountFactor->getValue(forModel), gradientBegin, gradientEnd, curveType);
		 m_foreignSpotFxDateDiscountFactor->accumulateGradient(forModel, - multiplier * getValue(domModel,forModel,fx)/m_foreignSpotFxDateDiscountFactor->getValue(forModel), gradientBegin, gradientEnd, curveType);  
	 }
	 
	 void DiscountedForwardRateNotionalExchange::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
										double multiplier,
                                        double fx, 
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread) 
	 {
		if(spread)
		{   
			double fixingBaseDF = dfModel.getBaseDiscountFactor(m_foreignFixingDateDiscountFactor->getArguments().getFlowTime());
			double spotBaseDF = dfModel.getBaseDiscountFactor(m_foreignSpotFxDateDiscountFactor->getArguments().getFlowTime());
			dfModel.accumulateBaseDiscountFactorGradient(m_foreignFixingDateDiscountFactor->getArguments().getFlowTime(), multiplier * getValue(baseModel,dfModel,fx)/m_foreignFixingDateDiscountFactor->getValue(dfModel), gradientBegin,  gradientEnd);
			dfModel.accumulateBaseDiscountFactorGradient(m_foreignSpotFxDateDiscountFactor->getArguments().getFlowTime(), - multiplier * getValue(baseModel,dfModel,fx)/m_foreignSpotFxDateDiscountFactor->getValue(dfModel), gradientBegin,  gradientEnd);
			
			// m_foreignFixingDateDiscountFactor->accumulateGradient(dfModel, multiplier * fixingSpreadDF * getValue(baseModel,dfModel,fx)/m_foreignFixingDateDiscountFactor->getValue(dfModel), gradientBegin, gradientEnd);
			// m_foreignSpotFxDateDiscountFactor->accumulateGradient(dfModel, - multiplier * spotSpreadDF * getValue(baseModel,dfModel,fx)/m_foreignSpotFxDateDiscountFactor->getValue(dfModel), gradientBegin, gradientEnd);
		}
		else
		{
			double notional = getNotional(baseModel, dfModel, fx);
			double tmp = m_coverage * m_forwardRate->getValue(baseModel) ;
			double pv = getValue(baseModel,dfModel,fx);
			double df = m_discountFactor->getValue(baseModel);
			m_discountFactor->accumulateGradient(baseModel, multiplier * (1.0 + tmp) * notional, gradientBegin, gradientEnd);
			m_startDateDiscountFactor->accumulateGradient(baseModel, - multiplier * notional, gradientBegin, gradientEnd);
			m_forwardRate->accumulateGradient(baseModel, multiplier * m_coverage * notional * df, gradientBegin, gradientEnd);

			m_domesticFixingDateDiscountFactor->accumulateGradient(baseModel, - multiplier * pv/m_domesticFixingDateDiscountFactor->getValue(baseModel), gradientBegin, gradientEnd);
			m_domesticSpotFxDateDiscountFactor->accumulateGradient(baseModel, multiplier * pv/m_domesticSpotFxDateDiscountFactor->getValue(baseModel), gradientBegin, gradientEnd);
		}
	 }
	 

	 void DiscountedForwardRateNotionalExchange::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
										double multiplier,
                                        double fx, 
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread) 
	{
		throw ModelException("DiscountedForwardRateNotionalExchange::accumulateGradientConstantTenorDiscountFactor", "Not implemented");
	}

	double DiscountedForwardRateNotionalExchange::getRate(BaseModel const& domModel, BaseModel const& forModel, double spotFx)
	{
		return m_forwardRate->getValue(domModel);
	}

    void DiscountedForwardRateNotionalExchange::update()
    {
        m_arguments.getForwardRate()->update();
        m_arguments.getDiscountFactor()->update();
        m_arguments.getStartDateDiscountFactor()->update();
        m_arguments.getForeignStartDateDiscountFactor()->update();
        m_arguments.getDomesticFixingDateDiscountFactor()->update();
        m_arguments.getForeignFixingDateDiscountFactor()->update();
		m_arguments.getDomesticSpotFxDateDiscountFactor()->update();
        m_arguments.getForeignSpotFxDateDiscountFactor()->update();
    }
	
    ostream& DiscountedForwardRateNotionalExchange::print(ostream& out) const
    {
        out << m_arguments << endl;
        return out;
    }

    ICloneLookupPtr DiscountedForwardRateNotionalExchange::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new DiscountedForwardRateNotionalExchange(*this, lookup));
    }

    DiscountedForwardRateNotionalExchange::DiscountedForwardRateNotionalExchange(DiscountedForwardRateNotionalExchange const& original, CloneLookup& lookup) : 
        m_arguments(original.m_arguments, lookup),
        m_forwardRate(lookup.get(original.m_forwardRate)),
        m_discountFactor(lookup.get(original.m_discountFactor)),
        m_startDateDiscountFactor(lookup.get(original.m_startDateDiscountFactor)),
        m_foreignStartDateDiscountFactor(lookup.get(original.m_foreignStartDateDiscountFactor)),
        m_domesticFixingDateDiscountFactor(lookup.get(original.m_domesticFixingDateDiscountFactor)),
        m_foreignFixingDateDiscountFactor(lookup.get(original.m_foreignFixingDateDiscountFactor)),
		m_domesticSpotFxDateDiscountFactor(lookup.get(original.m_domesticSpotFxDateDiscountFactor)),
        m_foreignSpotFxDateDiscountFactor(lookup.get(original.m_foreignSpotFxDateDiscountFactor)),
        m_coverage(original.m_coverage)
    {
    }
	
		void DiscountedForwardRateNotionalExchange::fillRepFlows(IDeA::AssetDomainConstPtr assetDomainDom,
                                  const BaseModel& domModel,
								  IDeA::AssetDomainConstPtr assetDomainFor,
                                  const BaseModel& forModel,
								  double spot,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
		{
			 double notional = getNotional(domModel, forModel, spot);
			 double pv = getValue(domModel,forModel,spot);

			fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomainDom,m_discountFactor->getArguments().getPayDate()),  multiplier * ( 1.0 + m_coverage * m_forwardRate->getValue(domModel) ) * notional);
			fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomainDom,m_startDateDiscountFactor->getArguments().getPayDate()),  - multiplier * notional);
			
			fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomainDom,m_domesticSpotFxDateDiscountFactor->getArguments().getPayDate()),  multiplier * pv/m_domesticSpotFxDateDiscountFactor->getValue(domModel));
			fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomainDom,m_domesticFixingDateDiscountFactor->getArguments().getPayDate()),- multiplier * pv/m_domesticFixingDateDiscountFactor->getValue(domModel));
			fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomainFor,m_foreignSpotFxDateDiscountFactor->getArguments().getPayDate()), - multiplier * pv/m_foreignSpotFxDateDiscountFactor->getValue(forModel));
			fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomainFor,m_foreignFixingDateDiscountFactor->getArguments().getPayDate()),   multiplier * pv/m_foreignFixingDateDiscountFactor->getValue(forModel));
		}
		void DiscountedForwardRateNotionalExchange::fillRepFlows(IDeA::AssetDomainConstPtr assetDomainDom,
                                  const BaseModel& domModel,
								  IDeA::AssetDomainConstPtr assetDomainFor,
                                  const BaseModel& forModel,
								  double spot,
								  const double multiplier,
                                  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
		{
			double notional = getNotional(domModel, forModel, spot);
			m_forwardRate->fillRepFlows(assetDomainDom, domModel, multiplier * m_coverage *  notional * m_discountFactor->getValue(domModel), indexRepFlows);
		}

}   //  FlexYCF