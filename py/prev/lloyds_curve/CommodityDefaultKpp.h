#ifndef __LIBRARY_PRICERS_FLEXYCF_COMMODITYDEFAULTKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_COMMODITYDEFAULTKPP_H_INCLUDED
#pragma once

#include "SingleCurveKpp.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( CommodityDefaultKpp )

    class CommodityDefaultKpp : public SingleCurveKpp
    {
    public:
        static std::string getName()
        {
            return "CommodityDefaultKpp";
        }
        
        static CommodityDefaultKppPtr createInstance(const LTQuant::GenericDataPtr)
        {
            return CommodityDefaultKppPtr(new CommodityDefaultKpp);
        }
        
        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel);
    };  

}   // FlexYCf

#endif //__LIBRARY_PRICERS_FLEXYCF_COMMODITYDEFAULTKPP_H_INCLUDED