#include "stdafx.h" 

//	FlexYCF
#include "CTDModel.h"
#include "FlexYCFCloneLookup.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"
#include "DoNothingSolver.h"


using namespace LTQC;
using namespace LTQuant;
using namespace IDeA;

namespace FlexYCF
{

    CTDModel::CTDModel(const LT::date& valueDate, const string& tenorDescription, const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent) :
        StripperModel(valueDate,tenorDescription,masterTable, parent), m_baseCcy(), m_baseCcyIndex(), m_baseXccyIndex(), m_modelCcyForCollateral(false), m_isBaseCcyModelCcy(true), m_isBaseCcyCollateral(false), m_initialized(false)
    {   
        const GenericDataPtr paramTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		m_ccy = IDeA::extract<std::string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY));	
		m_index = IDeA::extract<std::string>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX));	

		// enforce doNothing solver and initialization to zero
		const GenericDataPtr modelTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS)));
		LT::Str solver = IDeA::extract<std::string>(*modelTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, SOLVER));
		if(solver.compareCaseless(DoNothingSolver::getName()) != 0)
		{
			LTQC_THROW( IDeA::ModelException, "In model parameters for CTD model set solver to:  " << DoNothingSolver::getName().data());
		}
		const GenericDataPtr interpTable(IDeA::extract<GenericDataPtr>(*modelTable, IDeA_KEY(FLEXYC_MODELPARAMETERS,CURVESINTERPOLATION)));
		LTQuant::GenericDataPtr interpolationDetailsTable;
		interpTable->permissive_get<LTQuant::GenericDataPtr>(CurveType::_3M()->getDescription(), 0, interpolationDetailsTable, LTQuant::GenericDataPtr());
		if(interpolationDetailsTable)
		{
			double initRate = 1.0;
			bool hasInitRate = interpolationDetailsTable->permissive_get<double>("Init Rate", 0, initRate, 1.0);	
			if(!hasInitRate || initRate != 0.0)
			{
				LTQC_THROW( IDeA::ModelException, "In CTD model, in curve interpolation set Init Rate to 0.0");
			}
		}
		else
		{
			LTQC_THROW( IDeA::ModelException, "In CTD model, can not find " << CurveType::_3M()->getDescription() << " curve");
		}

		// CTD ADs
		const GenericDataPtr collateralParams(IDeA::extract<GenericDataPtr>(*paramTable, IDeA_KEY(YC_CURVEPARAMETERS, CTDCOLLATERAL)));
		LT::TablePtr baseCcy;
		IDeA::permissive_extract<LT::TablePtr>(collateralParams->table, IDeA_KEY(CTDCOLLATERAL, BASECURRENCY),baseCcy,LT::TablePtr());
		if(baseCcy)
		{
			m_baseCcy = IDeA::extract<LT::Str>(*baseCcy, IDeA_KEY(BASECURRENCY,CURRENCY));
			
			m_isBaseCcyModelCcy = m_baseCcy.compareCaseless(m_ccy) == 0;
			if(!m_isBaseCcyModelCcy)
			{	
				m_baseCcyIndex = IDeA::extract<LT::Str>(*baseCcy, IDeA_KEY(BASECURRENCY,BASECURVEINDEX));
				m_baseXccyIndex = IDeA::extract<LT::Str>(*baseCcy, IDeA_KEY(BASECURRENCY,SPREADCURVEINDEX));
			}
		}

		LT::TablePtr ctdCcys = IDeA::extract<LT::TablePtr>(collateralParams->table, IDeA_KEY(CTDCOLLATERAL, ELIGIBLECOLLATERAL));
		
		if(ctdCcys->rowsGet() < 2)
		{
			LTQC_THROW( IDeA::ModelException, "CDTModel: at least one collateral currency needed, but none provided");
		}

		for(size_t i = 0; i < ctdCcys->rowsGet()-1; ++i)
		{
			LT::Str ccyStr = IDeA::extract<LT::Str>(ctdCcys, IDeA_KEY(ELIGIBLECOLLATERAL, CURRENCY), i);
			LT::Str oisIndexStr = IDeA::extract<LT::Str>(ctdCcys, IDeA_KEY(ELIGIBLECOLLATERAL, COLLATERALINDEX), i);
			LT::Str csaIndexStr = IDeA::extract<LT::Str>(ctdCcys, IDeA_KEY(ELIGIBLECOLLATERAL, CSAINDEX), i);
			LT::Str xccyBaseIndexStr;
			IDeA::permissive_extract<LT::Str>(ctdCcys, IDeA_KEY(ELIGIBLECOLLATERAL, BASEINDEX), i,xccyBaseIndexStr,oisIndexStr);

			if( ccyStr.compareCaseless(m_ccy) == 0 )
			{
				std::string tmp;
				IRAssetDomain::buildDiscriminator(m_ccy,oisIndexStr,tmp);
				m_modelCollateralAD = AssetDomain::createAssetDomain(LT::Str(tmp));
				m_modelCcyForCollateral = true;
			}
			else
			{
				if(m_baseCcy.empty() || ccyStr.compareCaseless(m_baseCcy) != 0 )
				{
					std::string tmp;
					IRAssetDomain::buildDiscriminator(ccyStr,oisIndexStr,tmp);
					AssetDomainConstPtr ad = AssetDomain::createAssetDomain(LT::Str(tmp));
					m_collateralADs.push_back(ad);
			
					std::string tmp1;
					IRAssetDomain::buildDiscriminator(ccyStr,xccyBaseIndexStr,tmp1);
					AssetDomainConstPtr ad1 = AssetDomain::createAssetDomain(LT::Str(tmp1));
					m_xccyBaseADs.push_back(ad1);
				
					std::string tmp2;
					IRAssetDomain::buildDiscriminator(m_baseCcy.empty() ? m_ccy : ccyStr,csaIndexStr,tmp2);
					AssetDomainConstPtr ad2 = AssetDomain::createAssetDomain(LT::Str(tmp2));
					m_xccyADs.push_back(ad2);
				}
			}
			if(ccyStr.compareCaseless(m_baseCcy) == 0 )
			{
				std::string tmp;
				IRAssetDomain::buildDiscriminator(ccyStr,oisIndexStr,tmp);
				m_collateralBaseAD = AssetDomain::createAssetDomain(LT::Str(tmp));
				m_isBaseCcyCollateral = true;
			}
		}
    }

    BaseModelPtr CTDModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        return BaseModelPtr(new CTDModel(valueDate, CurveType::_3M()->getDescription(), masterTable, parent));
    }
     
	void CTDModel::onInitialized()
    {
        StripperModel::onInitialized();
		if(!m_initialized)
		{
			initializeModels();
			populateRates();
		    m_initialized = true;
		}
    }
    void CTDModel::populateRates() const
	{
		std::vector<double> nodes = abscissas();
		std::vector<double> dfs;
		double lastDf = 1.0;
		double start = 0.0;
		auto it = nodes.begin();
		if(*it == 0.0)
		{
			++it;
		}

		LTQC::VectorDouble y(nodes.size()-1);
		for(size_t i=0; it != nodes.end(); ++it, ++i)
		{
			double df = findCTDdf(start, *it);
			start = *it;
			lastDf *= df; 
			y[i] = getVariableValueFromSpineDiscountFactor(*it,lastDf);
		}
		const_cast<CTDModel*>(this)->updateVariablesFromShifts(y);
	}

    void CTDModel::initializeModels() const
    {
        LT::Str market; 
        if( !getDependentMarketData() )
        {
            LTQC_THROW( IDeA::ModelException, "CDTModel: unable to find any dependencies");
        }
        for (size_t i = 1; i < getDependentMarketData()->table->rowsGet(); ++i) 
        {
            AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
            LT::Str asset = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
            if (adType == IDeA::AssetDomainType::IR )
            {
                market = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
				if(hasDependentModel(IRAssetDomain(asset, market)))
				{
					string tmp;
					IRAssetDomain::buildDiscriminator(asset,market,tmp);
					AssetDomainConstPtr ad = AssetDomain::createAssetDomain(LT::Str(tmp));
					if(std::find(m_xccyADs.begin(), m_xccyADs.end(), ad) !=  m_xccyADs.end())
					{
						m_xccyModels[ad] = getDependentModel(IRAssetDomain(asset, market));
					}
					else if(std::find(m_collateralADs.begin(), m_collateralADs.end(), ad) !=  m_collateralADs.end())
					{
						m_collateralModels[ad] = getDependentModel(IRAssetDomain(asset, market));
					}

					if(std::find(m_xccyBaseADs.begin(), m_xccyBaseADs.end(), ad) !=  m_xccyBaseADs.end())
					{
						m_xccyBaseModels[ad] = getDependentModel(IRAssetDomain(asset, market));
					}	
					
					if(m_modelCcyForCollateral && asset.compareCaseless(m_ccy) == 0 && market.compareCaseless(m_modelCollateralAD->secondaryDomain()) == 0)
					{
						m_collateralModel = getDependentModel(IRAssetDomain(asset, market));
					}

					if(asset.compareCaseless(m_baseCcy) == 0 && market.compareCaseless(m_baseCcyIndex) == 0)
					{
						m_baseModel = getDependentModel(IRAssetDomain(asset, market));
					}
					else if(asset.compareCaseless(m_ccy) == 0 && market.compareCaseless(m_baseXccyIndex) == 0)
					{
						m_xccyBaseModel = getDependentModel(IRAssetDomain(asset, market));
					}
					if(m_collateralBaseAD && asset.compareCaseless(m_collateralBaseAD->primaryDomain()) == 0 && market.compareCaseless(m_collateralBaseAD->secondaryDomain()) == 0)
					{
						m_collateralBaseModel = getDependentModel(IRAssetDomain(asset, market));
					}
				}
            }
        }
		if( m_collateralModels.size() != m_xccyModels.size() || m_collateralModels.size() != m_collateralADs.size() || (m_modelCcyForCollateral && !m_collateralModel) || (m_isBaseCcyCollateral && !m_baseModel  && !m_xccyBaseModel) ||(!m_isBaseCcyModelCcy && !m_baseModel  && !m_xccyBaseModel)|| (m_isBaseCcyCollateral  && !m_collateralBaseModel))
		{
			LTQC_THROW( IDeA::ModelException, "CDTModel: unable to find enough dependencies");
		}
    }
		
		
		
	double CTDModel::impliedForwardRate(double startDate, double endDate, BaseModelConstPtr collateralModel) const
	{
		double dfStart = collateralModel->getDiscountFactor(startDate);
		double dfEnd = collateralModel->getDiscountFactor(endDate);

		return dfStart/dfEnd - 1.0;
	}

	double CTDModel::impliedForwardDf(double startDate, double endDate, BaseModelConstPtr collateralModel, BaseModelConstPtr xccyModel, BaseModelConstPtr baseModel) const
	{
		double baseDfStart = baseModel->getDiscountFactor(startDate);
		double baseDfEnd = baseModel->getDiscountFactor(endDate);

		double xccyDfStart = xccyModel->getDiscountFactor(startDate);
		double xccyDfEnd = xccyModel->getDiscountFactor(endDate);

		double xccy = xccyDfEnd/xccyDfStart;
		double base = baseDfEnd/baseDfStart;

		return 1.0/(2.0 + impliedForwardRate(startDate,endDate,collateralModel) -  xccy/base);
	}

	double CTDModel::impliedForwardDf(double startDate, double endDate, BaseModelConstPtr collateralModel, BaseModelConstPtr xccyModel, BaseModelConstPtr xccyModelAD, BaseModelConstPtr baseModel) const
	{	
		double baseDfStart = baseModel->getDiscountFactor(startDate);
		double baseDfEnd = baseModel->getDiscountFactor(endDate);

		double xccyDfStart = xccyModel->getDiscountFactor(startDate);
		double xccyDfEnd = xccyModel->getDiscountFactor(endDate);

		double xccyADDfStart = xccyModelAD->getDiscountFactor(startDate);
		double xccyADDfEnd = xccyModelAD->getDiscountFactor(endDate);

		double xccy1 = xccyDfEnd/xccyDfStart;
		double base = baseDfEnd/baseDfStart;
		double xccy2 = xccyADDfEnd/xccyADDfStart;
		
		return 1.0/(1.0 + impliedForwardRate(startDate,endDate,collateralModel) +  xccy1/base - xccy2/base);
	}

	
	double CTDModel::findCTDdf(double startDate, double endDate) const
	{
		double df = 1000.0;
		if(m_modelCcyForCollateral)
		{
			df = 1.0/(1.0 + impliedForwardRate(startDate, endDate, m_collateralModel));
		}
		
		auto it = m_collateralModels.begin();
		auto itX = m_xccyModels.begin();
		auto itB = m_xccyBaseModels.begin();
		for( ;it != m_collateralModels.end(); ++it, ++itX)
		{
			if(m_baseModel && m_xccyBaseModel && m_ccy.compareCaseless(itX->first->primaryDomain()) != 0)
			{
				df = std::min(impliedForwardDf(startDate, endDate, it->second, itX->second, m_xccyBaseModel, m_baseModel),df);
			}
			else
			{	
				df = std::min(impliedForwardDf(startDate, endDate, it->second, itX->second, itB->second),df);
				++itB;
			}
		}

		if(m_isBaseCcyCollateral)
		{
			df = std::min(impliedForwardDf(startDate, endDate, m_collateralBaseModel, m_xccyBaseModel, m_baseModel),df);
		}
		return df;
	}

	ICloneLookupPtr CTDModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new CTDModel(*this, lookup));
    }

   
    CTDModel::CTDModel(const CTDModel& original, CloneLookup& lookup) :
    StripperModel(original, lookup), 
		m_ccy(original.m_ccy),
		m_index(original.m_index),
		m_baseCcy(original.m_baseCcy),
		m_baseCcyIndex(original.m_baseCcyIndex),
		m_baseXccyIndex(original.m_baseXccyIndex),
		m_modelCcyForCollateral(original.m_modelCcyForCollateral),
		m_isBaseCcyModelCcy(original.m_isBaseCcyModelCcy),
		m_isBaseCcyCollateral(original.m_isBaseCcyCollateral),
		m_collateralADs(original.m_collateralADs), 
		m_xccyADs(original.m_xccyADs),
		m_xccyBaseADs(original.m_xccyBaseADs),
		m_modelCollateralAD(original.m_modelCollateralAD),
		m_collateralBaseAD(original.m_collateralBaseAD),
		m_collateralModels(original.m_collateralModels),
        m_xccyModels(original.m_xccyModels),
		m_xccyBaseModels(original.m_xccyBaseModels),
		m_baseModel(original.m_baseModel),
		m_xccyBaseModel(original.m_xccyBaseModel),
		m_collateralModel(original.m_collateralModel),
		m_collateralBaseModel(original.m_collateralBaseModel),
		m_initialized(original.m_initialized)
    {
    }

}	