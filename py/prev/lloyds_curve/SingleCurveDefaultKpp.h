/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEDEFAULTKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEDEFAULTKPP_H_INCLUDED
#pragma once

#include "SingleCurveKpp.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( SingleCurveDefaultKpp )

    /// SingleCurveDefaultKpp is concrete class that provides a default 
    /// implementation of single curve knot-point placement that can 
    /// be used by any single curve model.
    class SingleCurveDefaultKpp : public SingleCurveKpp
    {
    public:
        static std::string getName()
        {
            return "SingleCurveDefault";
        }
        
        static SingleCurveDefaultKppPtr createInstance(const LTQuant::GenericDataPtr)
        {
            return SingleCurveDefaultKppPtr(new SingleCurveDefaultKpp);
        }
        
        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel);
    };  // DefaultSingleCurveKPP

}   // FlexYCf

#endif //__LIBRARY_PRICERS_FLEXYCF_SINGLECURVEDEFAULTKPP_H_INCLUDED