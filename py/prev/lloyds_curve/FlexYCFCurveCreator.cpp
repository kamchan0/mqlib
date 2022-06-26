/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

#include "stdafx.h"

// FlexYCF
#include "FlexYCFCurveCreator.h"
#include "GenericIRMarketData.h"


// IDeA
#include "DataExtraction.h"
#include "DictMarketData.h"
#include "DictYieldCurve.h"
#include "BaseModelFactory.h"
#include "MultiYieldCurve.h"
#include "AssetDomain.h"
#include "FundingPolicy.h"
#include "MultiZeroCurve.h"

// LTQuantLib
#include "Data\MarketData\YieldCurveCreator.h"
#include "Data\GenericData.h"
#include "Library\Pricers\ZeroCurve.h"
#include "lt/const_string.h"


using namespace LTQC;

namespace LTQuant
{

    bool FlexYCFCurveCreator::requiresDependentMarketObjects(const LT::Table& data)
    {
        bool hasDependentMarketData = FlexYCFCurveCreator::isSwapSpreadIRCurve(data);
        if (!hasDependentMarketData) {
            // check if it has 
            LT::TablePtr curveParams = IDeA::extract<LT::TablePtr>(data, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS));
            LT::TablePtr depMktData;
            hasDependentMarketData = IDeA::permissive_extract<LT::TablePtr>(curveParams, IDeA_KEY(YC_CURVEPARAMETERS, YC_DEPENDENTMARKETDATA), depMktData, depMktData);
            if (hasDependentMarketData && depMktData)
                hasDependentMarketData = depMktData->rowsGet() > 1;
        }
        return hasDependentMarketData;
    }
        
    bool FlexYCFCurveCreator::isSwapSpreadIRCurve(const LT::Table& data)
    {
        // This is a SwapSpreadIRCurve if:
        //  - model is not a FlexYCF model
        //  - curve has only Ccy Basis or Spread instruments


        bool isSwapSpreadIRCurveOnly = true;     
        LT::Str modelName;
        IDeA::permissive_extract<LT::Str>(data, IDeA_KEY(YIELDCURVE, MODEL), modelName, modelName);

        if (!modelName.empty())
        {
            // This is not a FlexYCF curve if Model given: check if it is a valid FlexYCF model
            isSwapSpreadIRCurveOnly = !FlexYCF::ModelFactory::isValidModel(modelName.string());
        } 

        if (isSwapSpreadIRCurveOnly) {
            // It's not a valid FlexYCF model. So it can be a SwapSpreadIRModel if it has only Ccy Basis or Spread instruments
            LT::TablePtr instrumentList = IDeA::extract<LT::TablePtr>(data, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST));

            if (!instrumentList || instrumentList->rowsGet()==0)
                LT_THROW_ERROR("Instrument table is empty");

            if (instrumentList->colsGet() == 1) {
                isSwapSpreadIRCurveOnly = (instrumentList->findColKey(LT::NoThrowT(),"Ccy Basis")!= LT::Table::not_found) ? true : false;
                // just to retain previous behaviour
                //isSwapSpreadIRCurveOnly = IDeA::keyExists(*instrumentList, IDeA_KEY(YC_INSTRUMENTLIST, CCYBASIS)); 
                if (!isSwapSpreadIRCurveOnly) {
                    // Check for Spread key for backward compatibility
                    isSwapSpreadIRCurveOnly = (instrumentList->findColKey(LT::NoThrowT(),"Spread")!= LT::Table::not_found) ? true : false;
                }
            }
			else isSwapSpreadIRCurveOnly = false;
        }
        return isSwapSpreadIRCurveOnly;
    }

	bool FlexYCFCurveCreator::isSwapSpreadIRCurve(const GenericDataPtr& data) const
	{
        if (!data)
            LT_THROW_ERROR("Null data in spread curve");

        if (!data->table)
            LT_THROW_ERROR("Null data in spread curve");


        return FlexYCFCurveCreator::isSwapSpreadIRCurve(*data->table);
	}

	bool FlexYCFCurveCreator::isFlexYCF(const GenericDataPtr& data) const
	{
        // this is a FlexYCF curve if it is a valid FlexYCF model

        bool isFlexYCF = false;

        // Check if it is a FlexYCF model
        std::string modelName;
        IDeA::permissive_extract<std::string>(*data, IDeA_KEY(YIELDCURVE, MODEL), modelName);

        if (!modelName.empty())
        {
            // Model given: check if it is a valid FlexYCF model
            isFlexYCF = FlexYCF::ModelFactory::isValidModel(modelName);
        } 

		//if (isFlexYCF) {
            // this is a FlexYCF model but check that 
        //    isFlexYCF = !isSwapSpreadIRCurve(data);
        //}

        return isFlexYCF;
	}

    // factory class for yield curves
    // default to old behaviour if the Table Curve Settings is missing
    MarketDataPtr FlexYCFCurveCreator::create(GenericDataPtr data)
    {
        //	GenericDataPtr details = data->get<GenericDataPtr>("Curve Details", 0);
		const GenericDataPtr details(IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		
		//	date buildDate = details->get<date>("Build Date", 0);
		const LT::date buildDate(IDeA::extract<LT::date>(*details, IDeA_KEY(CURVEDETAILS, BUILDDATE)));

        const string& type(details->get<string>("Type", 0));

		if (LT::StrRef(type.data()).compareCaseless("IL") == 0) 
		{
			string ccyStr = details->get<string>("Asset", 0);
			string market = details->get<string>("Market", 0);
			string assetName(ccyStr + "." + market);

			/*
			std::string staticDataTag;
			bool staticDataTagExists = true;

			if(data->doesTagExist("Static Data"))
			{
				staticDataTag = "Static Data";
			}
			else if(data->doesTagExist("StaticData"))
			{
				staticDataTag = "StaticData";
			}
			else
			{
				staticDataTagExists = false;
			}
			if (!staticDataTagExists)
				LT_THROW_ERROR("Can't find static data for curve type " << type);
			GenericDataPtr staticTable(data->get<GenericDataPtr>(staticDataTag, 0));
			
			*/
			GenericDataPtr staticTable;
			try
			{
				staticTable = IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(INFLATIONCURVE, STATICDATA));
			}
			catch(...)
			{
				LT_THROW_ERROR("Can't find static data for curve type " << type);
			}

			GenericDataPtr assetTable(staticTable->get<GenericDataPtr>("Assets", 0));
			bool found = false;
			size_t assetEntry = 0;
			for(size_t i(0); !found && i < assetTable->numItems() - 1; ++i)
			{
				if(compareNoCase(assetTable->get<string>("Name", i), assetName))
				{
					assetEntry = i;
					found = true;
				}
			}
			if (!found) 
			{
				LT_THROW_ERROR("Can't find asset " << assetName);
			}

			GenericDataPtr thisAssetTable = assetTable->get<GenericDataPtr>("Asset", assetEntry);

			string currencyName= thisAssetTable->get<string>("Currency", 0);
			string indexName = thisAssetTable->get<string>("Index", 0);; 

			Index index(currencyName, Index::INFLATION_INDEX, indexName);

			// create Market data object that simply wraps the underlying table
			// note we don't pass the data object.
			// this gets set outside
			GenericIRMarketDataPtr md(new GenericIRMarketData(buildDate, index));
			return md;
			
		} 
		else if (LT::StrRef(type.data()).compareCaseless("IR") == 0)
        {
			const GenericDataPtr parameters(IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));

	        Index index(IDeA::extract<string>(*parameters, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY)),
						Index::INTEREST_RATE,
						IDeA::extract<string>(*parameters, IDeA_KEY(YC_CURVEPARAMETERS, FORECASTINDEX)));

			GenericDataPtr instrumentList(IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST)));
		
			//	Nicolas: 
			//	What's the use of this "topLeft" here?! Did someone forget to delete it?
			//	Is it here for the purpose if throwing an error if instrumentList is empty?
			
			//	string topLeft(instrumentList->get<string>(0, 0));

			GenericIRMarketDataPtr md(new GenericIRMarketData(buildDate, index));
			md->setData(data);
			return md;
        }
		else
			LT_THROW_ERROR("Invalid type " << type);

		return GenericIRMarketDataPtr();
			
    }
    /*
    * Create a compositeCurveRepresentation that can be used in a priceSupplier (that is outside IDeA
    * 
    * @ccy the currency this curve is a spread to as the composite descriptor may not be self describing
    * @param index the index is the composite curve 
    * @return zero curve representation of the composite curve
    */
     ZeroCurvePtr FlexYCFCurveCreator::createCompositeCurve(const LT::Str& ccy, const LTQuant::Index& index, LTQuant::PriceSupplierPtr& priceSupplier)
     {
         //first index is always the composite, that is the interface
         IDeA::AssetDomainConstPtr genericAD=IDeA::AssetDomain::createAssetDomain(index.getIndexName());
         IDeA::IRCompositeAssetDomainConstPtr compAD=LT::dynamicCast<const IDeA::IRCompositeAssetDomain,const IDeA::AssetDomain>(genericAD);

         IDeA::FundingPolicyPtr fp=IDeA::FundingPolicy::createCompositeFundingPolicyFromPS(ccy,compAD,priceSupplier);

         LTQuant::ZeroCurvePtr ltqlzc(new IDeA::MultiZeroCurve(priceSupplier.get(), fp)); 
        
         return ltqlzc;
     }
}
