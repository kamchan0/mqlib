#include "stdafx.h"
#include "CommodityDefaultKpp.h"
#include "BaseModel.h"
#include "StripperModel.h"
#include "CommodityFutures.h"
#include "Data\GenericData.h"
#include "DictYieldCurve.h"
#include "DataExtraction.h"
#include "AssetDomain.h"
#include "MarketObject.h"
#include "DateUtils.h"
#include "FxSpot.h"

using namespace LTQC;
using namespace std;
using namespace IDeA;

namespace FlexYCF
{
 
    bool CommodityDefaultKpp::createKnotPoints(const CalibrationInstruments& instruments, 
                                                 const BaseModelPtr baseModel)
    {
		const StripperModelPtr singleCurve(std::tr1::dynamic_pointer_cast<StripperModel>(baseModel));
    
        vector<std::string> fxPairs;
        for(size_t i(0); i < instruments.size(); ++i)
        {
            if( instruments[i]->getLastRelevantTime() > 0.0 )
            {
                KnotPoint kp(instruments[i]->getLastRelevantTime(), 200.0, false, instruments[i]);
                singleCurve->addKnotPoint(kp);
            }
            if( instruments[i]->getType() == FlexYCF::CommodityFutures::getName())
		    {
				FlexYCF::CommodityFuturesConstPtr future( std::tr1::dynamic_pointer_cast< const FlexYCF::CommodityFutures>( instruments[i] ) );

                fxPairs.push_back( future->getCommodityCurrencyPair() );
			}

        }
        if( fxPairs.size() == 0 )
        {
            LT_THROW_ERROR( "CommodityDefaultKpp can not find any commodity futures");
        }
        const std::string& fxPair = fxPairs[0];		
        if( std::count(fxPairs.begin(), fxPairs.end(), fxPair) != fxPairs.size() )
        {
            LT_THROW_ERROR( "CommodityDefaultKpp: all commodity futures should have the same underlyning asset and currency");
        }

        // add spot rate
        LT::Str domCurrency;
        LT::Str forCurrency;
        IDeA::FxSpot::getCurrencies(domCurrency, forCurrency, fxPair);

        double fxSpotRate = 0.0;
        fxSpotRate = baseModel->getDependentFXRate(FXSpotAssetDomain(forCurrency, domCurrency));

        LT::date valueDate = baseModel->getValueDate();
        LT::date fxSpotDate = LTQuant::getFXSpotDate(valueDate, fxPair);
        double spotTime = ModuleDate::getYearsBetween(valueDate, fxSpotDate);
        singleCurve->setSpotRate(spotTime, fxSpotRate);

        baseModel->update();

        return true;
    }
    
}