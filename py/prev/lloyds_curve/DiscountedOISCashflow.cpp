#include "stdafx.h"

// IDeA
#include "AssetDomain.h"
#include "Exception.h"

//	FlexYCF
#include "DiscountedOISCashflow.h"
#include "DiscountedForwardRateArguments.h"
#include "GenericInstrumentComponent.h"
#include "BaseModel.h"
#include "ForwardRateArguments.h"
#include "ForwardRate.h"
#include "FlexYcfUtils.h"
#include "RepFlowsData.h"
#include "FlexYCFCloneLookup.h"

#include "AssetDomain.h"
#include "DictYieldCurve.h"

using namespace std;

using namespace LTQC;
using namespace IDeA;

namespace FlexYCF
{
    double DiscountedOISCashflow::getValue(BaseModel const& baseModel)
    {
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double startDf = m_startDateTenorDiscountFactor->getValue(baseModel);
		double endDf = m_endDateTenorDiscountFactor->getValue(baseModel);
		double payDf = m_payDiscountFactor->getValue(baseModel);
        return covRatio * ( startDf - endDf ) * payDf/endDf;            
    }

    // As the discounted forward rate (DFR), is cov/covON * (startDf - endDf) * payDf/endDf, its gradient can be computed as:
    //  grad(DFR) = ((grad(startDf) -grad(endDf))*payDf/endDf + (startDf-endDf)*(grad(payDf)*endDf - grad(endDf)*payDf)/endDf^2) * cov/covON 
    void DiscountedOISCashflow::accumulateGradient(BaseModel const& baseModel, 
												   double multiplier, 
												   GradientIterator gradientBegin, 
												   GradientIterator gradientEnd)
    {
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double startDf = m_startDateTenorDiscountFactor->getValue(baseModel);
		double endDf = m_endDateTenorDiscountFactor->getValue(baseModel);
		double payDf = m_payDiscountFactor->getValue(baseModel);
        double payOverEnd = payDf/endDf;

        m_startDateTenorDiscountFactor->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd  , gradientBegin, gradientEnd);
		m_endDateTenorDiscountFactor->accumulateGradient(baseModel,   covRatio * multiplier * (-payOverEnd - (startDf - endDf)*payOverEnd/endDf)  , gradientBegin, gradientEnd);
		m_payDiscountFactor->accumulateGradient(baseModel,   covRatio * multiplier * (startDf - endDf)/endDf  , gradientBegin, gradientEnd);
    }

	 void DiscountedOISCashflow::accumulateGradient(BaseModel const& baseModel, 
													double multiplier,
													GradientIterator gradientBegin,
													GradientIterator gradientEnd, 
													const CurveTypeConstPtr& curveType)
	 {
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double startDf = m_startDateTenorDiscountFactor->getValue(baseModel);
		double endDf = m_endDateTenorDiscountFactor->getValue(baseModel);
		double payDf = m_payDiscountFactor->getValue(baseModel);
        double payOverEnd = payDf/endDf;
		
		m_startDateTenorDiscountFactor->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd  , gradientBegin, gradientEnd, curveType);
		m_endDateTenorDiscountFactor->accumulateGradient(baseModel,   covRatio * multiplier * (-payOverEnd - (startDf - endDf)*payOverEnd/endDf)  , gradientBegin, gradientEnd, curveType);
		m_payDiscountFactor->accumulateGradient(baseModel,   covRatio * multiplier * (startDf - endDf)/endDf  , gradientBegin, gradientEnd, curveType);
	 }
	
	void DiscountedOISCashflow::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double startDf = m_startDateTenorDiscountFactor->getValue(baseModel);
		double endDf = m_endDateTenorDiscountFactor->getValue(baseModel);
		double payDf = m_payDiscountFactor->getValue(dfModel);
        double payOverEnd = payDf/endDf;

        m_startDateTenorDiscountFactor->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd  , gradientBegin, gradientEnd);
		m_endDateTenorDiscountFactor->accumulateGradient(baseModel,   - covRatio * multiplier * startDf * payOverEnd / endDf  , gradientBegin, gradientEnd);
		if( spread )
		{
			double  spreadDf = dfModel.getSpreadDiscountFactor(m_payDiscountFactor->getArguments().getFlowTime());
			m_payDiscountFactor->accumulateGradient(baseModel, multiplier * covRatio * (startDf/endDf - 1.0) * spreadDf,gradientBegin, gradientEnd);
		}
	}
	void DiscountedOISCashflow::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double startDf = m_startDateTenorDiscountFactor->getValue(dfModel);
		double endDf = m_endDateTenorDiscountFactor->getValue(dfModel);
		double payDf = m_payDiscountFactor->getValue(baseModel);
        double payOverEnd = payDf/endDf;

		m_startDateTenorDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, covRatio * multiplier * payOverEnd  , gradientBegin, gradientEnd, spread);
		m_endDateTenorDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel,   - covRatio * multiplier * startDf * payOverEnd / endDf  , gradientBegin, gradientEnd,spread);
	
		m_payDiscountFactor->accumulateGradient(baseModel, multiplier * covRatio * ( startDf/endDf - 1.0 ), gradientBegin, gradientEnd);
	}

	double DiscountedOISCashflow::getRate(const BaseModel& model)
	{
		return (m_startDateTenorDiscountFactor->getValue(model)/m_endDateTenorDiscountFactor->getValue(model) - 1.0)/m_arguments.getCoverageON();
	}

    void DiscountedOISCashflow::update()
    {
        m_startDateTenorDiscountFactor->update();
		m_endDateTenorDiscountFactor->update();
		m_payDiscountFactor->update();
    }

	void DiscountedOISCashflow::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                             const BaseModel& model,
											 const double multiplier,
											 IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		//	add (df date, mult * cvg * fwd rate) pair
        fundingRepFlows.addRepFlow(IDeA::Funding::Key(model.primaryAssetDomainFunding(), m_payDiscountFactor->getArguments().getPayDate()), multiplier * m_arguments.getCoverage() * getRate(model) );
	}

	void DiscountedOISCashflow::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                             const BaseModel& model,
											 const double multiplier,
											 IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
        double covRatioTimesDf = m_arguments.getCoverage()/m_arguments.getCoverageON() * m_payDiscountFactor->getValue(model);
		indexRepFlows.addRepFlow(IDeA::Index::Key(model.primaryAssetDomainIndex(), m_startDateTenorDiscountFactor->getArguments().getPayDate(), m_startDateTenorDiscountFactor->getArguments().getTenor()), multiplier * covRatioTimesDf / m_endDateTenorDiscountFactor->getValue(model));
		indexRepFlows.addRepFlow(IDeA::Index::Key(model.primaryAssetDomainIndex(), m_endDateTenorDiscountFactor->getArguments().getPayDate(),m_endDateTenorDiscountFactor->getArguments().getTenor()),  -multiplier * covRatioTimesDf * m_startDateTenorDiscountFactor->getValue(model) / pow(m_endDateTenorDiscountFactor->getValue(model), 2.0));
	}

    ostream& DiscountedOISCashflow::print(ostream& out) const
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
    ICloneLookupPtr DiscountedOISCashflow::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new DiscountedOISCashflow(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Use a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
   DiscountedOISCashflow::DiscountedOISCashflow(DiscountedOISCashflow const& original, CloneLookup& lookup) : 
        m_arguments(original.m_arguments, lookup),
        m_startDateTenorDiscountFactor(lookup.get(original.m_startDateTenorDiscountFactor)),
        m_endDateTenorDiscountFactor(lookup.get(original.m_endDateTenorDiscountFactor)),
		m_payDiscountFactor(lookup.get(original.m_payDiscountFactor))
    {
    }
}   //  FlexYCF

namespace FlexYCF
{
    double FundingDiscountedCashflow::getValue(BaseModel const& baseModel)
    {
        if( !m_fundingFwdModel )
        {
            initialize(baseModel);
        }
		double startDf = m_startDateDiscountFactor->getValue(*m_fundingFwdModel);
		double endDf = m_endDateDiscountFactor->getValue(*m_fundingFwdModel);
        double payDf = m_payDateDiscountFactor->getValue(baseModel);
        return (startDf/endDf - 1.0) * payDf;            
    }

   
    void FundingDiscountedCashflow::accumulateGradient(BaseModel const& baseModel, 
												   double multiplier, 
												   GradientIterator gradientBegin, 
												   GradientIterator gradientEnd)
    {
        m_payDateDiscountFactor->accumulateGradient(baseModel, multiplier * m_arguments.getCoverage() * getRate(baseModel), gradientBegin, gradientEnd);
    }

	 void FundingDiscountedCashflow::accumulateGradient(BaseModel const& baseModel, 
													double multiplier,
													GradientIterator gradientBegin,
													GradientIterator gradientEnd, 
													const CurveTypeConstPtr& curveType)
	 {
		m_payDateDiscountFactor->accumulateGradient(baseModel, multiplier * m_arguments.getCoverage() * getRate(baseModel), gradientBegin, gradientEnd, curveType);
     }
	 
	 void FundingDiscountedCashflow::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		
		double startDf = m_startDateDiscountFactor->getValue(baseModel);
		double endDf = m_endDateDiscountFactor->getValue(baseModel);
        double payDf = m_payDateDiscountFactor->getValue(dfModel);
          
		m_startDateDiscountFactor->accumulateGradient(baseModel, multiplier * payDf / endDf  , gradientBegin, gradientEnd);
		m_endDateDiscountFactor->accumulateGradient(baseModel,   - multiplier * startDf * payDf /( endDf * endDf)  , gradientBegin, gradientEnd);	
		if( spread )
		{
			double  spreadDf = dfModel.getSpreadDiscountFactor(m_payDateDiscountFactor->getArguments().getFlowTime());
			m_payDateDiscountFactor->accumulateGradient(baseModel, multiplier * (startDf/endDf - 1.0) * spreadDf,gradientBegin, gradientEnd);
		}
	}
	
	 void FundingDiscountedCashflow::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		double startDf = m_startDateDiscountFactor->getValue(dfModel);
		double endDf = m_endDateDiscountFactor->getValue(dfModel);
		double payDf = m_payDateDiscountFactor->getValue(baseModel);
        double payOverEnd = payDf/endDf;

		m_startDateDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier * payOverEnd  , gradientBegin, gradientEnd, spread);
		m_endDateDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel,   - multiplier * startDf * payOverEnd / endDf  , gradientBegin, gradientEnd,spread);
	
		m_payDateDiscountFactor->accumulateGradient(baseModel, multiplier * ( startDf/endDf - 1.0 ), gradientBegin, gradientEnd);
	}
	
	double FundingDiscountedCashflow::getRate(const BaseModel& model)
	{
        if( !m_fundingFwdModel )
        {
            initialize(model);
        }
		return (m_startDateDiscountFactor->getValue(*m_fundingFwdModel)/m_endDateDiscountFactor->getValue(*m_fundingFwdModel) - 1.0)/m_arguments.getCoverage();
	}

    void FundingDiscountedCashflow::update()
    {
        m_startDateDiscountFactor->update();
		m_endDateDiscountFactor->update();
        m_payDateDiscountFactor->update();
    }

	void FundingDiscountedCashflow::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                             const BaseModel& model,
											 const double multiplier,
											 IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		//	add (df date, mult * cvg * fwd rate) pair
        fundingRepFlows.addRepFlow(IDeA::Funding::Key(model.primaryAssetDomainFunding(), m_payDateDiscountFactor->getArguments().getPayDate()), multiplier * m_arguments.getCoverage() * getRate(model) );
		if( !m_fundingFwdModel )
        {
            initialize(model);
        }
		double startDf = m_startDateDiscountFactor->getValue(*m_fundingFwdModel);
		double endDf = m_endDateDiscountFactor->getValue(*m_fundingFwdModel);
        double payDf = m_payDateDiscountFactor->getValue(model);
    
        fundingRepFlows.addRepFlow(IDeA::Funding::Key(m_fundingFwdModelAD, m_startDateDiscountFactor->getArguments().getPayDate()), multiplier * payDf/endDf);
		fundingRepFlows.addRepFlow(IDeA::Funding::Key(m_fundingFwdModelAD, m_endDateDiscountFactor->getArguments().getPayDate()), - multiplier * startDf * payDf/(endDf * endDf) );
	}

	void FundingDiscountedCashflow::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                             const BaseModel& model,
											 const double multiplier,
											 IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
	}

    ostream& FundingDiscountedCashflow::print(ostream& out) const
    {
        out << m_arguments << endl;
        return out;
    }

    ICloneLookupPtr FundingDiscountedCashflow::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new FundingDiscountedCashflow(*this, lookup));
    }

  
   FundingDiscountedCashflow::FundingDiscountedCashflow(FundingDiscountedCashflow const& original, CloneLookup& lookup) : 
        m_arguments(original.m_arguments, lookup),
        m_startDateDiscountFactor(lookup.get(original.m_startDateDiscountFactor)),
        m_endDateDiscountFactor(lookup.get(original.m_endDateDiscountFactor)),
        m_payDateDiscountFactor(lookup.get(original.m_payDateDiscountFactor))
    {
    }

    void FundingDiscountedCashflow::initialize(const BaseModel& model) const
    {
        if( !m_fundingFwdModel )
        {
            LT::Str market;
            if( !model.getDependentMarketData() )
            {
                LTQC_THROW( IDeA::ModelException, "FundingDiscountedCashflow: unable to find any dependencies");
            }
            for (size_t i = 1; i < model.getDependentMarketData()->table->rowsGet(); ++i) 
            {
                AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                LT::Str asset = IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
                if (adType == IDeA::AssetDomainType::IR )
                {
                    if( asset.compareCaseless(m_arguments.getCurrency()) == 0 )
                    {
                        market = IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
                        break;
                    }
                }
            }
                
            if(model.hasDependentModel(IRAssetDomain( m_arguments.getCurrency(), market)))
            {
				string tmp;
				IRAssetDomain::buildDiscriminator(m_arguments.getCurrency(),market,tmp);
				m_fundingFwdModelAD = AssetDomain::createAssetDomain(LT::Str(tmp));
                m_fundingFwdModel = model.getDependentModel(IRAssetDomain( m_arguments.getCurrency(), market));
            }
            else
            {
                LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model for " << m_arguments.getCurrency().data());
            }
        }
    }

}   //  FlexYCF


namespace FlexYCF
{
    double DiscountedArithmeticOISCashflow::getValue(BaseModel const& baseModel)
    {
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double payDf = m_payDiscountFactor->getValue(baseModel);

		double tmp = 0.0;
		double startDf = m_endDatesTenorDiscountFactor[0]->getValue(baseModel);
		for(size_t i=1; i < m_endDatesTenorDiscountFactor.size(); ++i)
		{
			double endDf = m_endDatesTenorDiscountFactor[i]->getValue(baseModel);
			
			if(i != m_endDatesTenorDiscountFactor.size() - 1)
				tmp += startDf/endDf - 1.0;
			else
				tmp += m_cutoffAdj * (startDf/endDf - 1.0);
			
			startDf = endDf;
		}
		return covRatio * tmp * payDf;
    }

    // As the discounted forward rate (DFR), is cov/covON * (startDf - endDf) * payDf/endDf, its gradient can be computed as:
    //  grad(DFR) = ((grad(startDf) -grad(endDf))*payDf/endDf + (startDf-endDf)*(grad(payDf)*endDf - grad(endDf)*payDf)/endDf^2) * cov/covON 
    void DiscountedArithmeticOISCashflow::accumulateGradient(BaseModel const& baseModel, 
												   double multiplier, 
												   GradientIterator gradientBegin, 
												   GradientIterator gradientEnd)
    {
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double payDf = m_payDiscountFactor->getValue(baseModel);
        
		for(size_t i=1; i<m_endDatesTenorDiscountFactor.size(); ++i)
		{
			double startDf = m_endDatesTenorDiscountFactor[i-1]->getValue(baseModel);
			double endDf = m_endDatesTenorDiscountFactor[i]->getValue(baseModel);
			double payOverEnd = payDf/endDf;
			if(i != m_endDatesTenorDiscountFactor.size() - 1)
			{
				m_endDatesTenorDiscountFactor[i-1]->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd  , gradientBegin, gradientEnd);
				m_endDatesTenorDiscountFactor[i]->accumulateGradient(baseModel,  -covRatio * multiplier * payOverEnd * startDf / endDf  , gradientBegin, gradientEnd);
			}
			else
			{
				m_endDatesTenorDiscountFactor[i-1]->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd * m_cutoffAdj, gradientBegin, gradientEnd);
				m_endDatesTenorDiscountFactor[i]->accumulateGradient(baseModel,  -covRatio * multiplier * payOverEnd * startDf / endDf * m_cutoffAdj, gradientBegin, gradientEnd);
			}
		}
		m_payDiscountFactor->accumulateGradient(baseModel,   multiplier * getValue(baseModel)/payDf  , gradientBegin, gradientEnd);
    }

	 void DiscountedArithmeticOISCashflow::accumulateGradient(BaseModel const& baseModel, 
													double multiplier,
													GradientIterator gradientBegin,
													GradientIterator gradientEnd, 
													const CurveTypeConstPtr& curveType)
	 {
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double payDf = m_payDiscountFactor->getValue(baseModel);
        
		for(size_t i=1; i<m_endDatesTenorDiscountFactor.size(); ++i)
		{
			double startDf = m_endDatesTenorDiscountFactor[i-1]->getValue(baseModel);
			double endDf = m_endDatesTenorDiscountFactor[i]->getValue(baseModel);
			double payOverEnd = payDf/endDf;
			if(i != m_endDatesTenorDiscountFactor.size() - 1)
			{
				m_endDatesTenorDiscountFactor[i-1]->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd  , gradientBegin, gradientEnd, curveType);
				m_endDatesTenorDiscountFactor[i]->accumulateGradient(baseModel,  -covRatio * multiplier * payOverEnd * startDf / endDf  , gradientBegin, gradientEnd, curveType);
			}
			else
			{
				m_endDatesTenorDiscountFactor[i-1]->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd * m_cutoffAdj, gradientBegin, gradientEnd,curveType);
				m_endDatesTenorDiscountFactor[i]->accumulateGradient(baseModel,  -covRatio * multiplier * payOverEnd * startDf / endDf * m_cutoffAdj, gradientBegin, gradientEnd,curveType);
			}
		}
		m_payDiscountFactor->accumulateGradient(baseModel,   multiplier * getValue(baseModel)/payDf  , gradientBegin, gradientEnd, curveType);
	 }
	
	void DiscountedArithmeticOISCashflow::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, bool spread)
	{
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double payDf = m_payDiscountFactor->getValue(baseModel);
        
		for(size_t i=1; i<m_endDatesTenorDiscountFactor.size(); ++i)
		{
			double startDf = m_endDatesTenorDiscountFactor[i-1]->getValue(baseModel);
			double endDf = m_endDatesTenorDiscountFactor[i]->getValue(baseModel);
			double payOverEnd = payDf/endDf;
			if(i != m_endDatesTenorDiscountFactor.size() - 1)
			{
				m_endDatesTenorDiscountFactor[i-1]->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd  , gradientBegin, gradientEnd);
				m_endDatesTenorDiscountFactor[i]->accumulateGradient(baseModel,  -covRatio * multiplier * payOverEnd * startDf / endDf  , gradientBegin, gradientEnd);
			}
			else
			{
				m_endDatesTenorDiscountFactor[i-1]->accumulateGradient(baseModel, covRatio * multiplier * payOverEnd  * m_cutoffAdj, gradientBegin, gradientEnd);
				m_endDatesTenorDiscountFactor[i]->accumulateGradient(baseModel,  -covRatio * multiplier * payOverEnd * startDf / endDf  * m_cutoffAdj, gradientBegin, gradientEnd);
			}
		}
		if( spread )
		{
			double  spreadDf = dfModel.getSpreadDiscountFactor(m_payDiscountFactor->getArguments().getFlowTime());
			m_payDiscountFactor->accumulateGradient(baseModel,  multiplier * getValue(baseModel) / payDf * spreadDf,gradientBegin, gradientEnd);
		}
	}


	void DiscountedArithmeticOISCashflow::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, bool spread)
	{
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double payDf = m_payDiscountFactor->getValue(baseModel);
        
		for(size_t i=1; i<m_endDatesTenorDiscountFactor.size(); ++i)
		{
			double startDf = m_endDatesTenorDiscountFactor[i-1]->getValue(baseModel);
			double endDf = m_endDatesTenorDiscountFactor[i]->getValue(baseModel);
			double payOverEnd = payDf/endDf;
			if(i != m_endDatesTenorDiscountFactor.size() - 1)
			{
				m_endDatesTenorDiscountFactor[i-1]->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, covRatio * multiplier * payOverEnd  , gradientBegin, gradientEnd, spread);
				m_endDatesTenorDiscountFactor[i]->accumulateGradientConstantTenorDiscountFactor(baseModel,  dfModel, -covRatio * multiplier * payOverEnd * startDf / endDf  , gradientBegin, gradientEnd, spread);
			}
			else
			{
				m_endDatesTenorDiscountFactor[i-1]->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, covRatio * multiplier * payOverEnd  * m_cutoffAdj, gradientBegin, gradientEnd, spread);
				m_endDatesTenorDiscountFactor[i]->accumulateGradientConstantTenorDiscountFactor(baseModel,  dfModel, -covRatio * multiplier * payOverEnd * startDf / endDf * m_cutoffAdj, gradientBegin, gradientEnd, spread);
			}
		}
		m_payDiscountFactor->accumulateGradient(baseModel,  multiplier * getValue(baseModel) / payDf, gradientBegin, gradientEnd);
	}

	double DiscountedArithmeticOISCashflow::getRate(const BaseModel& baseModel)
	{
		double tmp = 0.0;
		double startDf = m_endDatesTenorDiscountFactor[0]->getValue(baseModel);
		for(size_t i=1; i<m_endDatesTenorDiscountFactor.size(); ++i)
		{
			double endDf = m_endDatesTenorDiscountFactor[i]->getValue(baseModel);
			if(i != m_endDatesTenorDiscountFactor.size() - 1)
			{
				tmp += startDf/endDf - 1.0;
			}
			else
			{
				tmp += m_cutoffAdj * (startDf/endDf - 1.0);
			}
			startDf = endDf;
		}
		return tmp/m_arguments.getCoverageON();
	}

    void DiscountedArithmeticOISCashflow::update()
    {
        m_startDateTenorDiscountFactor->update();
		m_endDateTenorDiscountFactor->update();
		m_payDiscountFactor->update();

		for(size_t i=0; i<m_endDatesTenorDiscountFactor.size(); ++i)
		{
			m_endDatesTenorDiscountFactor[i]->update();
		}
    }

	void DiscountedArithmeticOISCashflow::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                             const BaseModel& model,
											 const double multiplier,
											 IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
        fundingRepFlows.addRepFlow(IDeA::Funding::Key(model.primaryAssetDomainFunding(), m_payDiscountFactor->getArguments().getPayDate()), multiplier * m_arguments.getCoverage() * getRate(model) );
	}

	void DiscountedArithmeticOISCashflow::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                             const BaseModel& model,
											 const double multiplier,
											 IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		double covRatio = m_arguments.getCoverage()/m_arguments.getCoverageON();
		double payDf = m_payDiscountFactor->getValue(model);
        
		for(size_t i=1; i<m_endDatesTenorDiscountFactor.size(); ++i)
		{
			double startDf = m_endDatesTenorDiscountFactor[i-1]->getValue(model);
			double endDf = m_endDatesTenorDiscountFactor[i]->getValue(model);
			double payOverEnd = payDf/endDf;
			if(i != m_endDatesTenorDiscountFactor.size() - 1)
			{
				indexRepFlows.addRepFlow(IDeA::Index::Key(model.primaryAssetDomainIndex(), m_endDatesTenorDiscountFactor[i-1]->getArguments().getPayDate(), m_endDatesTenorDiscountFactor[i-1]->getArguments().getTenor()), multiplier * covRatio * payOverEnd);
				indexRepFlows.addRepFlow(IDeA::Index::Key(model.primaryAssetDomainIndex(), m_endDatesTenorDiscountFactor[i]->getArguments().getPayDate(),m_endDatesTenorDiscountFactor[i]->getArguments().getTenor()),  -multiplier * covRatio * payOverEnd * startDf / endDf);
			}
			else
			{
				indexRepFlows.addRepFlow(IDeA::Index::Key(model.primaryAssetDomainIndex(), m_endDatesTenorDiscountFactor[i-1]->getArguments().getPayDate(), m_endDatesTenorDiscountFactor[i-1]->getArguments().getTenor()), multiplier * covRatio * payOverEnd * m_cutoffAdj);
				indexRepFlows.addRepFlow(IDeA::Index::Key(model.primaryAssetDomainIndex(), m_endDatesTenorDiscountFactor[i]->getArguments().getPayDate(),m_endDatesTenorDiscountFactor[i]->getArguments().getTenor()),  -multiplier * covRatio * payOverEnd * startDf / endDf * m_cutoffAdj);
			}
		}	
	}

    ostream& DiscountedArithmeticOISCashflow::print(ostream& out) const
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
    ICloneLookupPtr DiscountedArithmeticOISCashflow::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new DiscountedArithmeticOISCashflow(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Use a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
   DiscountedArithmeticOISCashflow::DiscountedArithmeticOISCashflow(DiscountedArithmeticOISCashflow const& original, CloneLookup& lookup) : 
        m_arguments(original.m_arguments, lookup),
        m_startDateTenorDiscountFactor(lookup.get(original.m_startDateTenorDiscountFactor)),
        m_endDateTenorDiscountFactor(lookup.get(original.m_endDateTenorDiscountFactor)),
		m_payDiscountFactor(lookup.get(original.m_payDiscountFactor)),
		m_cutoffAdj(original.m_cutoffAdj)
    {
		for(size_t i=0; i<m_endDatesTenorDiscountFactor.size(); ++i)
		{
			m_endDatesTenorDiscountFactor[i] = lookup.get(original.m_endDatesTenorDiscountFactor[i]);
		}
    }
}   //  FlexYCF