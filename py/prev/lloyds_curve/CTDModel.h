#ifndef __LIBRARY_PRICERS_FLEXYCF_CTDMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CTDMODEL_H_INCLUDED
#pragma once

//	FlexYCF
#include "StripperModel.h"

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
    class CTDModel : public StripperModel
    {
    public:
        CTDModel(const LT::date& valueDate, const string& tenorDescription, const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent);

        static std::string getName()
        {
            return "CTDStripper";
        }

        static BaseModelPtr createInstance(const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent);

        double getDiscountFactor(const double flowTime) const
        {
            return StripperModel::getDiscountFactor(flowTime);
        }
        
        double getTenorDiscountFactor(const double flowTime, const double tenor) const
        {
           return StripperModel::getDiscountFactor(flowTime);
        }

		void accumulateSpreadDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const {}
		void accumulateBaseDiscountFactorGradient(const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const {}
		void accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const {}
		void accumulateBaseTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const {}                                                      
        ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
	   
		void onInitialized();
    
	protected:
        CTDModel(CTDModel const& original, CloneLookup& lookup);
        
	private:
		
		void populateRates() const;
        void initializeModels() const;	
		double impliedForwardRate(double startDate, double endDate, BaseModelConstPtr collateralModel) const;
		double impliedForwardDf(double startDate, double endDate, BaseModelConstPtr collateralModel, BaseModelConstPtr xccyModel, BaseModelConstPtr baseModel) const;
		double impliedForwardDf(double startDate, double endDate, BaseModelConstPtr collateralModel, BaseModelConstPtr xccyModel, BaseModelConstPtr xccyModelAD, BaseModelConstPtr baseModel) const;
		double findCTDdf(double startDate, double endDate) const;
        CTDModel(CTDModel const&); 
    
        LT::Str                                 m_ccy;
        LT::Str                                 m_index;
        LT::Str                                 m_baseCcy;
		LT::Str									m_baseCcyIndex;
		LT::Str									m_baseXccyIndex;
		bool									m_modelCcyForCollateral;
		bool									m_isBaseCcyModelCcy;
		bool									m_isBaseCcyCollateral;
		std::vector<IDeA::AssetDomainConstPtr>  m_collateralADs;
		std::vector<IDeA::AssetDomainConstPtr>  m_xccyADs;
		std::vector<IDeA::AssetDomainConstPtr>  m_xccyBaseADs;
		IDeA::AssetDomainConstPtr			    m_modelCollateralAD;
		IDeA::AssetDomainConstPtr			    m_collateralBaseAD;

		mutable std::map<IDeA::AssetDomainConstPtr, BaseModelConstPtr, IDeA::AssetDomainConstPtrLessThan>   m_collateralModels;
		mutable std::map<IDeA::AssetDomainConstPtr, BaseModelConstPtr, IDeA::AssetDomainConstPtrLessThan>   m_xccyModels;
		mutable std::map<IDeA::AssetDomainConstPtr, BaseModelConstPtr, IDeA::AssetDomainConstPtrLessThan>   m_xccyBaseModels;
		mutable BaseModelConstPtr					    										m_baseModel;
		mutable BaseModelConstPtr					    										m_xccyBaseModel;
		mutable BaseModelConstPtr					    										m_collateralModel;
		mutable BaseModelConstPtr					    										m_collateralBaseModel;
		mutable bool													                        m_initialized;
    }; 

    DECLARE_SMART_PTRS( CTDModel )
}


#endif //__LIBRARY_PRICERS_FLEXYCF_CTDMODEL_H_INCLUDED