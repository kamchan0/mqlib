/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_STRIPPERNOCASHENDDATEKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_STRIPPERNOCASHENDDATEKPP_H_INCLUDED
#pragma once

#include "SingleCurveKpp.h"
#include "utils/EnumBase.h"
#include "KnotPlacementUtils.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( StripperNoCashEndDateKpp )  
 
    /// StripperNoCashEndDateKpp place knot pointsor 
    /// a StripperModel as follows: futures and cash end
    /// dates are taken, except when the cash and first futures
    /// overlap, in which case the cash end date is ignore and
    /// the first futures start date is taken instead.
    class StripperNoCashEndDateKpp : public SingleCurveKpp
    {
    public:
        explicit StripperNoCashEndDateKpp(const LT::date valueDate, CashSelection cashSelection);

        static std::string getName();

        /// Creates a StripperNoCashEndDateKpp
        static StripperNoCashEndDateKppPtr createInstance(const LTQuant::GenericDataPtr masterTable);

        virtual void selectInstruments(CalibrationInstruments& instruments, 
                                       const BaseModelPtr baseModel);
        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel);
    
    private:
        LT::date m_valueDate;
        CashSelection          m_cashSelection;
    };  //  StripperNoCashEndDateKpp

    DECLARE_SMART_PTRS( StripperNoCashEndDateKpp )

}   // FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_STRIPPERNOCASHENDDATEKPP_H_INCLUDED