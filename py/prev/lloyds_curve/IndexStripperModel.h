#ifndef __LIBRARY_PRICERS_FLEXYCF_INDEXSTRIPPERMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INDEXSTRIPPERMODEL_H_INCLUDED
#pragma once

//	FlexYCF
#include "StripperModel.h"

// #include "UtilsEnums.h"
#include "lt\ptr.h"
#include "BaseModel.h"
#include "DataExtraction.h"
#include "DictYieldCurve.h"
#include "AssetDomain.h"
#include "MarketObject.h"
#include "FactoryEnvelopeH.h"
#include "YieldCurve.h"
#include "TblConversion.h"
#include "MultiCurveModel.h"

namespace FlexYCF
{
    class IndexSpreadStripperModel : public StripperModel
    {
    public:
        explicit IndexSpreadStripperModel(const LT::date valueDate, 
                               const string tenorDescription,
                               const LTQuant::GenericDataPtr masterTable,
                               const LTQuant::FlexYCFZeroCurvePtr parent);

        static std::string getName()
        {
            return "IndexSpreadStripper";
        }

        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent); 

        virtual double getDiscountFactor(const double flowTime) const
        {
            if(!m_dependentModel)
            {
                initialize();
            }
            return m_dependentModel->getDiscountFactor(flowTime);
        }
        
        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            if(!m_dependentModel)
            {
                initialize();
            } 
            double tdf = m_dependentModel->getTenorDiscountFactor(flowTime,tenor);
            return tdf * StripperModel::getDiscountFactor(flowTime);
        }
        
		virtual double getBaseTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            if(!m_dependentModel)
            {
                initialize();
            } 
            return m_dependentModel->getTenorDiscountFactor(flowTime,tenor);
        } 
		
		virtual double getSpreadTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            return StripperModel::getDiscountFactor(flowTime);
        }

        void accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
		
		IDeA::AssetDomainConstPtr  primaryAssetDomainFunding()   const 
		{ 
			if(!m_dependentModel)
			{
				initialize();
			}
			return m_dependentModelAD;
		}
    protected:
        IndexSpreadStripperModel(IndexSpreadStripperModel const& original, CloneLookup& lookup);
        
        void initialize() const
        {
            LT::Str market; 
            if( !getDependentMarketData() )
            {
                LTQC_THROW( IDeA::ModelException, "IndexSpreadStripperModel: unable to find any dependencies");
            }
            size_t numberIRCurves = 0;
			for (size_t i = 1; i < getDependentMarketData()->table->rowsGet(); ++i) 
            {
                IDeA::AssetDomainType adType = IDeA::AssetDomainType(IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                LT::Str asset = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
                if (adType == IDeA::AssetDomainType::IR )
                {
					++numberIRCurves;
                    if( asset.compareCaseless(m_ccy) == 0 )
                    {
                        market = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
						 if(hasDependentModel(IDeA::IRAssetDomain(m_ccy, market)))
						{
							m_dependentModel = getDependentModel(IDeA::IRAssetDomain(m_ccy, market));
							string tmp;
							IDeA::IRAssetDomain::buildDiscriminator(m_ccy,market,tmp);
							m_dependentModelAD = IDeA::AssetDomain::createAssetDomain(LT::Str(tmp));
						}
                     
                    }
					else
					{
						market = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
						if(hasDependentModel(IDeA::IRAssetDomain(asset, market)))
						{
							m_leg2Model = getDependentModel(IDeA::IRAssetDomain(asset, market));
							string tmp;
							IDeA::IRAssetDomain::buildDiscriminator(asset,market,tmp);
							m_leg2ModelAD = IDeA::AssetDomain::createAssetDomain(LT::Str(tmp));
						}
					}
                }
            }
			if( numberIRCurves > 2)
            {
                LTQC_THROW( IDeA::ModelException, "Maximum of 2 dependent curves allowed");
            }   
			if(!m_dependentModel)
            {
                LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model for " << m_ccy.data() << " " << m_index.data());
            }   
		}

		const LTQC::Matrix& getFullJacobian() const;
    	
		LT::TablePtr getFullJacobian( const bool includeHeadings ) const
        {
            LT::TablePtr initialJacobian = LTQC::TblConversion_toTbl( getFullJacobian() );
           
            return initialJacobian;
        }
		
		virtual size_t jacobianOffset(IDeA::AssetDomainConstPtr ad) const
		{
			if(m_leg2ModelAD && ad->discriminator().compareCaseless(m_leg2ModelAD->discriminator()) == 0)
		   {
				return 0;
		   }
		   if(m_leg2ModelAD && ad->discriminator().compareCaseless(m_dependentModelAD->discriminator()) == 0)
		   {
				return m_leg2Model->numberOfPlacedInstruments();
		   }
		   if(!m_leg2ModelAD && ad->discriminator().compareCaseless(m_dependentModelAD->discriminator()) == 0)
		   {
				return 0;
		   }
		   if( m_leg2Model )
				return m_dependentModel->numberOfPlacedInstruments() + m_leg2Model->numberOfPlacedInstruments();
		   return 0;
	   }
		

    private:
        const LTQC::Matrix& getFullJacobian2() const;
        IndexSpreadStripperModel(IndexSpreadStripperModel const&); 
        mutable LTQC::Matrix				    m_fullJacobian;
        LT::Str                                 m_ccy;
        LT::Str                                 m_index;
	protected:
        mutable BaseModelConstPtr               m_dependentModel;
		mutable BaseModelConstPtr               m_leg2Model;
		mutable IDeA::AssetDomainConstPtr       m_dependentModelAD;
		mutable IDeA::AssetDomainConstPtr       m_leg2ModelAD;
    }; 

    DECLARE_SMART_PTRS( IndexSpreadStripperModel )

};


namespace FlexYCF
{
    class FundingIndexSpreadStripperModel : public StripperModel
    {
    public:
        explicit FundingIndexSpreadStripperModel(const LT::date valueDate, 
                               const string tenorDescription,
                               const LTQuant::GenericDataPtr masterTable,
                               const LTQuant::FlexYCFZeroCurvePtr parent);

        static std::string getName()
        {
            return "FundingIndexSpreadStripper";
        }

        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent); 

        virtual double getDiscountFactor(const double flowTime) const
        {
            if(!m_dependentModel)
            {
                initialize();
            }
            double bdf =  m_dependentModel->getDiscountFactor(flowTime);
			return bdf * StripperModel::getDiscountFactor(flowTime);
        }
        
        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            return getDiscountFactor(flowTime);
        }
		
		virtual double getTenorDiscountFactor(double flowTime, double tenor, const LTQC::Currency& ccy, const LT::Str& index) const;

        virtual double getBaseDiscountFactor(const double flowTime) const
        {
            if(!m_dependentModel)
            {
                initialize();
            }
            return m_dependentModel->getDiscountFactor(flowTime);
        }

		virtual double getSpreadDiscountFactor(const double flowTime) const
        {
            return StripperModel::getDiscountFactor(flowTime);
        }

		

        void accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        FundingIndexSpreadStripperModel(FundingIndexSpreadStripperModel const& original, CloneLookup& lookup);
        
        void initialize() const
        {
            LT::Str market; 
            if( !getDependentMarketData() )
            {
                LTQC_THROW( IDeA::ModelException, "IndexSpreadStripperModel: unable to find any dependencies");
            }
            size_t numberIRCurves = 0;
			for (size_t i = 1; i < getDependentMarketData()->table->rowsGet(); ++i) 
            {
                IDeA::AssetDomainType adType = IDeA::AssetDomainType(IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                LT::Str asset = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
                if (adType == IDeA::AssetDomainType::IR )
                {
					++numberIRCurves;
                    if( asset.compareCaseless(m_ccy) == 0 )
                    {
                        market = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
						 if(hasDependentModel(IDeA::IRAssetDomain(m_ccy, market)))
						{
							m_dependentModel = getDependentModel(IDeA::IRAssetDomain(m_ccy, market));
							string tmp;
							IDeA::IRAssetDomain::buildDiscriminator(m_ccy,market,tmp);
							m_dependentModelAD = IDeA::AssetDomain::createAssetDomain(LT::Str(tmp));
						}
                     
                    }
                }
            }
			if( numberIRCurves > 1)
            {
                LTQC_THROW( IDeA::ModelException, "Maximum of 1 dependent curves allowed");
            }   
			if(!m_dependentModel)
            {
                LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model for " << m_ccy.data() << " " << m_index.data());
            }   
		}

		const LTQC::Matrix& getFullJacobian() const;
    	
		LT::TablePtr getFullJacobian( const bool includeHeadings ) const
        {
            LT::TablePtr initialJacobian = LTQC::TblConversion_toTbl( getFullJacobian() );
           
            return initialJacobian;
        }
		
		virtual size_t jacobianOffset(IDeA::AssetDomainConstPtr ad) const
		{
		   if(ad->discriminator().compareCaseless(m_dependentModelAD->discriminator()) == 0)
		   {
				return 0;
		   }
		   return m_dependentModel->numberOfPlacedInstruments();
	   }
		

    private:
     
        FundingIndexSpreadStripperModel(FundingIndexSpreadStripperModel const&); 
        mutable LTQC::Matrix				    m_fullJacobian;
        LT::Str                                 m_ccy;
        LT::Str                                 m_index;
	protected:
        mutable BaseModelConstPtr               m_dependentModel;
		mutable IDeA::AssetDomainConstPtr       m_dependentModelAD;
	
    }; 

    DECLARE_SMART_PTRS( FundingIndexSpreadStripperModel )

};

namespace FlexYCF
{
    class IndexBaseSpreadStripperModel : public StripperModel
    {
    public:
        explicit IndexBaseSpreadStripperModel(const LT::date valueDate, 
                               const string tenorDescription,
                               const LTQuant::GenericDataPtr masterTable,
                               const LTQuant::FlexYCFZeroCurvePtr parent);

        static std::string getName()
        {
            return "IndexBaseSpreadStripper";
        }

        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent); 

        virtual double getDiscountFactor(const double flowTime) const
        {
            if(!m_dependentModel)
            {
                initialize();
            }
            return m_dependentModel->getDiscountFactor(flowTime);
        }
        
        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            if(!m_dependentModel)
            {
                initialize();
            } 
            double tdf = m_dependentModel->getTenorDiscountFactor(flowTime,m_baseRateTenor);
            return tdf * StripperModel::getDiscountFactor(flowTime);
        }
        
		virtual double getBaseTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            if(!m_dependentModel)
            {
                initialize();
            } 
            return m_dependentModel->getTenorDiscountFactor(flowTime,m_baseRateTenor);
        } 
		
		virtual double getSpreadTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            return StripperModel::getDiscountFactor(flowTime);
        }

        void accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
		
		IDeA::AssetDomainConstPtr  primaryAssetDomainFunding()   const 
		{ 
			if(!m_dependentModel)
			{
				initialize();
			}
			return m_dependentModelAD;
		}
    protected:
        IndexBaseSpreadStripperModel(IndexBaseSpreadStripperModel const& original, CloneLookup& lookup);
        
        void initialize() const
        {
            LT::Str market; 
            if( !getDependentMarketData() )
            {
                LTQC_THROW( IDeA::ModelException, "IndexBaseSpreadStripperModel: unable to find any dependencies");
            }
			size_t numberIRCurves = 0;
            for (size_t i = 1; i < getDependentMarketData()->table->rowsGet(); ++i) 
            {
                IDeA::AssetDomainType adType = IDeA::AssetDomainType(IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                LT::Str asset = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
                if (adType == IDeA::AssetDomainType::IR )
                {
					++numberIRCurves;
                    if( asset.compareCaseless(m_ccy) == 0 )
                    {
                        market = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
						 if(hasDependentModel(IDeA::IRAssetDomain(m_ccy, market)))
						{
							m_dependentModel = getDependentModel(IDeA::IRAssetDomain(m_ccy, market));
							string tmp;
							IDeA::IRAssetDomain::buildDiscriminator(m_ccy,market,tmp);
							m_dependentModelAD = IDeA::AssetDomain::createAssetDomain(LT::Str(tmp));
						}
                     
                    }
					else
					{
						market = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
						if(hasDependentModel(IDeA::IRAssetDomain(asset, market)))
						{
							m_leg2Model = getDependentModel(IDeA::IRAssetDomain(asset, market));
							string tmp;
							IDeA::IRAssetDomain::buildDiscriminator(asset,market,tmp);
							m_leg2ModelAD = IDeA::AssetDomain::createAssetDomain(LT::Str(tmp));
						}
					}
                }
            }
			if( numberIRCurves > 2)
            {
                LTQC_THROW( IDeA::ModelException, "Maximum of 2 dependent curves allowed");
            }   
			if(!m_dependentModel)
            {
                LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model for " << m_ccy.data() << " " << m_index.data());
            }   

		   const MultiCurveModelConstPtr multTenorModel(std::tr1::dynamic_pointer_cast<const MultiCurveModel>(m_dependentModel));
		   if( multTenorModel )
		   {
				m_baseRateTenor = multTenorModel->getBaseRate()->getYearFraction();
		   }
		   else
		   {
			   m_baseRateTenor = CurveType::_3M()->getYearFraction();
		   }
        }

		const LTQC::Matrix& getFullJacobian() const;
    	
		LT::TablePtr getFullJacobian( const bool includeHeadings ) const
        {
            LT::TablePtr initialJacobian = LTQC::TblConversion_toTbl( getFullJacobian() );
           
            return initialJacobian;
        }
		
		virtual size_t jacobianOffset(IDeA::AssetDomainConstPtr ad) const
		{
			if(m_leg2ModelAD && ad->discriminator().compareCaseless(m_leg2ModelAD->discriminator()) == 0)
		   {
				return 0;
		   }
		   if(m_leg2ModelAD && ad->discriminator().compareCaseless(m_dependentModelAD->discriminator()) == 0)
		   {
				return m_leg2Model->numberOfPlacedInstruments();
		   }
		   if(!m_leg2ModelAD && ad->discriminator().compareCaseless(m_dependentModelAD->discriminator()) == 0)
		   {
				return 0;
		   }
		   if( m_leg2Model )
				return m_dependentModel->numberOfPlacedInstruments() + m_leg2Model->numberOfPlacedInstruments();
		   return 0;
	   }
		
    private:
        const LTQC::Matrix& getFullJacobian2() const;
        IndexBaseSpreadStripperModel(IndexBaseSpreadStripperModel const&); 
        mutable LTQC::Matrix				    m_fullJacobian;
        LT::Str                                 m_ccy;
        LT::Str                                 m_index;
	
        mutable BaseModelConstPtr               m_dependentModel;
		mutable double m_baseRateTenor;
		mutable BaseModelConstPtr               m_leg2Model;
		mutable IDeA::AssetDomainConstPtr       m_dependentModelAD;
		mutable IDeA::AssetDomainConstPtr       m_leg2ModelAD;
    }; 

    DECLARE_SMART_PTRS( IndexBaseSpreadStripperModel )

}
#endif 