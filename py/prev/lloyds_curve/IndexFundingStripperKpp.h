#ifndef __LIBRARY_PRICERS_FLEXYCF_INDEXFUNDINGSTRIPPERKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INDEXFUNDINGSTRIPPERKPP_H_INCLUDED
#pragma once

#include "SingleCurveKpp.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( IndexFundingStripperKpp )

    class IndexFundingStripperKpp : public SingleCurveKpp
    {
    public:
        static std::string getName()
        {
            return "IndexFundingStripperKpp";
        }
        
        static IndexFundingStripperKppPtr createInstance(const LTQuant::GenericDataPtr)
        {
            return IndexFundingStripperKppPtr(new IndexFundingStripperKpp);
        }
        
        virtual bool createKnotPoints(const CalibrationInstruments& instruments, const BaseModelPtr baseModel);
    };  

}
#endif