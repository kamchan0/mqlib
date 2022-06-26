#ifndef __LIBRARY_PRICERS_FLEXYCF_FUNDINGSTRIPPERMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FUNDINGSTRIPPERMODEL_H_INCLUDED
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

namespace FlexYCF
{
    class FundingStripperModel : public StripperModel
    {
    public:
        explicit FundingStripperModel(const LT::date valueDate, 
                               const string tenorDescription,
                               const LTQuant::GenericDataPtr masterTable,
                               const LTQuant::FlexYCFZeroCurvePtr parent);

        static std::string getName()
        {
            return "FundingStripper";
        }

        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent);

        virtual double getDiscountFactor(const double flowTime) const
        {
            return StripperModel::getDiscountFactor(flowTime);
        }
        
        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            if(!m_dependentModel)
            {
                initialize();
            }
            return m_dependentModel->getTenorDiscountFactor(flowTime,tenor);
        }
       void accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
	   void accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
       void accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
       void accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;                                                      
       virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
	   
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
    
	protected:
        FundingStripperModel(FundingStripperModel const& original, CloneLookup& lookup);
        
        void initialize() const
        {
            LT::Str market; 
            if( !getDependentMarketData() )
            {
                LTQC_THROW( IDeA::ModelException, "FundingStripperModel: unable to find any dependencies");
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
    private:
        const LTQC::Matrix& getFullJacobian2() const;
        FundingStripperModel(FundingStripperModel const&); 
        mutable LTQC::Matrix				    m_fullJacobian;
        LT::Str                                 m_ccy;
        LT::Str                                 m_index;
        mutable BaseModelConstPtr               m_dependentModel;
		mutable BaseModelConstPtr               m_leg2Model;
		mutable IDeA::AssetDomainConstPtr       m_dependentModelAD;
		mutable IDeA::AssetDomainConstPtr       m_leg2ModelAD;
    }; 

    DECLARE_SMART_PTRS( FundingStripperModel )

};


namespace FlexYCF
{
    class FundingSpreadStripperModel : public StripperModel
    {
    public:
        explicit FundingSpreadStripperModel(const LT::date valueDate, 
                               const string tenorDescription,
                               const LTQuant::GenericDataPtr masterTable,
                               const LTQuant::FlexYCFZeroCurvePtr parent);

        static std::string getName()
        {
            return "FundingSpreadStripper";
        }

        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent); 

        virtual double getDiscountFactor(const double flowTime) const
        {
            if(!m_dependentModel)
            {
                initialize();
            }
            double df = m_dependentModel->getDiscountFactor(flowTime);
            return df * StripperModel::getDiscountFactor(flowTime);
        }
        
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

        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const
        {
            if(!m_dependentModel)
            {
                initialize();
            }
            return m_dependentModel->getTenorDiscountFactor(flowTime,tenor);
        }
         
        void accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		
		void accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
		
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
		
		IDeA::AssetDomainConstPtr  primaryAssetDomainIndex()   const 
		{ 
			if(!m_dependentModel)
			{
				initialize();
			}
			return m_dependentModelAD;
		}

    protected:
        FundingSpreadStripperModel(FundingSpreadStripperModel const& original, CloneLookup& lookup);
        
        void initialize() const
        {
            LT::Str market; 
            if( !getDependentMarketData() )
            {
                LTQC_THROW( IDeA::ModelException, "FundingSpreadStripperModel: unable to find any dependencies");
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

    private:
        const LTQC::Matrix& getFullJacobian2() const;
        FundingSpreadStripperModel(FundingSpreadStripperModel const&); 
        mutable LTQC::Matrix				    m_fullJacobian;
        LT::Str                                 m_ccy;
        LT::Str                                 m_index;
        mutable BaseModelConstPtr               m_dependentModel;
		mutable BaseModelConstPtr               m_leg2Model;
		mutable IDeA::AssetDomainConstPtr       m_dependentModelAD;
		mutable IDeA::AssetDomainConstPtr       m_leg2ModelAD;
    }; 

    DECLARE_SMART_PTRS( FundingSpreadStripperModel )

};


namespace FlexYCF
{
    class FundingSpreadModel : public StripperModel
    {
    public:
        explicit FundingSpreadModel(const LT::date valueDate, 
                               const string tenorDescription,
                               const LTQuant::GenericDataPtr masterTable,
                               const LTQuant::FlexYCFZeroCurvePtr parent);

        static std::string getName()
        {
            return "FundingIndexSpreadSingleCurve";
        }

        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent); 

        virtual double getDiscountFactor(const double flowTime) const
        {
            if(!m_dependentModel)
            {
                initialize();
            }
            double df = m_dependentModel->getDiscountFactor(flowTime);
            return df * StripperModel::getDiscountFactor(flowTime);
        }
        
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

        virtual double getTenorDiscountFactor(const double flowTime, const double tenor) const
        {
			if(m_isCalibrated)
				return getDiscountFactor(flowTime);

            if(!m_dependentModel)
            {
                initialize();
            }
            return m_dependentModel->getTenorDiscountFactor(flowTime,tenor);
        }
         
        void accumulateDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		
		void accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        void accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
		void accumulateSpreadTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const;
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
		
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
		
		IDeA::AssetDomainConstPtr  primaryAssetDomainIndex()   const 
		{ 
			if(!m_dependentModel)
			{
				initialize();
			}
			return m_dependentModelAD;
		}

	    void finishCalibration()
		{
			StripperModel::finishCalibration();
			m_isCalibrated = true;
		}

		void setCalibrated() { m_isCalibrated = true; }
    protected:
        FundingSpreadModel(FundingSpreadModel const& original, CloneLookup& lookup);
        
        void initialize() const
        {
            LT::Str market; 
            if( !getDependentMarketData() )
            {
                LTQC_THROW( IDeA::ModelException, "FundingSpreadModel: unable to find any dependencies");
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

	

    private:
        const LTQC::Matrix& getFullJacobian2() const;
        FundingSpreadModel(FundingSpreadModel const&); 
        mutable LTQC::Matrix				    m_fullJacobian;
        LT::Str                                 m_ccy;
        LT::Str                                 m_index;
        mutable BaseModelConstPtr               m_dependentModel;
		mutable BaseModelConstPtr               m_leg2Model;
		mutable IDeA::AssetDomainConstPtr       m_dependentModelAD;
		mutable IDeA::AssetDomainConstPtr       m_leg2ModelAD;
		bool									m_isCalibrated;
    }; 

    DECLARE_SMART_PTRS( FundingSpreadModel )

};




#endif //__LIBRARY_PRICERS_FLEXYCF_STRIPPERMODEL_H_INCLUDED