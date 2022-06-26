/*****************************************************************************

	StripperAllDatesKpp

    This Stripper knot-point placement:
	- discards non-base cash rates, fixed futures and swaps maturing
	  before the end date of the last futures
	- places a knot at the end date of the selected instruments

	Note: when there are no futures, all cash rates are selected and
	a knot is placed at their end dates.

    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_STRIPPERALLDATESKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_STRIPPERALLDATESKPP_H_INCLUDED
#pragma once
#include "SingleCurveKpp.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( StripperAllDatesKpp )

    /// StripperAllDatesKpp place knot-points for 
    /// a StripperModel at the cash end date, the
    /// first futures start and end dates and all 
    /// other futures end dates.
    class StripperAllDatesKpp : public SingleCurveKpp
    {
    public:
        static std::string getName();
        
        /// Creates a StripperAllDatesKpp
        static StripperAllDatesKppPtr createInstance(const LTQuant::GenericDataPtr)
        {
            return StripperAllDatesKppPtr(new StripperAllDatesKpp);
        }
		virtual void selectInstruments(CalibrationInstruments& instruments, 
                                       const BaseModelPtr baseModel);
        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel);

    };   //  StripperNoCashEndDateKpp

    DECLARE_SMART_PTRS( StripperAllDatesKpp )

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_STRIPPERALLDATESKPP_H_INCLUDED