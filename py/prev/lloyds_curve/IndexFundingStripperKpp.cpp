#include "stdafx.h"
#include "IndexFundingStripperKpp.h"
#include "BaseModel.h"
#include "FundingStripperModel.h"
#include "IndexStripperModel.h"
#include "CrossCurrencySwap.h"
#include "Data\GenericData.h"
#include "DictYieldCurve.h"
#include "DataExtraction.h"
#include "AssetDomain.h"
#include "MarketObject.h"
#include "DateUtils.h"
#include "FxSpot.h"

using namespace LTQC;

namespace FlexYCF
{
 
    bool IndexFundingStripperKpp::createKnotPoints(const CalibrationInstruments& instruments, const BaseModelPtr baseModel)
    {
       
		
        const StripperModelPtr singleCurve(std::tr1::dynamic_pointer_cast<StripperModel>(baseModel));
        const FundingSpreadStripperModelPtr fundingSpreadSingleCurve(std::tr1::dynamic_pointer_cast<FundingSpreadStripperModel>(baseModel));
		const FundingSpreadModelPtr fundingSpread(std::tr1::dynamic_pointer_cast<FundingSpreadModel>(baseModel));
        const IndexSpreadStripperModelPtr indexSpreadSingleCurve(std::tr1::dynamic_pointer_cast<IndexSpreadStripperModel>(baseModel));
		const FundingIndexSpreadStripperModelPtr fundingIndexSpreadSingleCurve(std::tr1::dynamic_pointer_cast<FundingIndexSpreadStripperModel>(baseModel));
        
        double defaultValue = 0.05;
        if(fundingSpreadSingleCurve || fundingSpread || indexSpreadSingleCurve || fundingIndexSpreadSingleCurve)
        {
            defaultValue = 0.0;
        }
        for(size_t i(0); i < instruments.size(); ++i)
        {
            if( instruments[i]->getLastRelevantTime() > 0.0 )
            {
                KnotPoint kp(instruments[i]->getLastRelevantTime(), defaultValue * instruments[i]->getLastRelevantTime(), false, instruments[i]);
                singleCurve->addKnotPoint(kp);
            }
        }
      
		baseModel->update();
        return true;
    }
    
}