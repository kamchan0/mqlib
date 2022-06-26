/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INFLATIONKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INFLATIONKPP_H_INCLUDED
#pragma once
#include "BaseKnotPointPlacement.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( InflationKpp )
	FWD_DECLARE_SMART_PTRS( InflationOnLastPaymentDateKpp )

    /// InflationKpp is the knot-point placement for InflationModel
    class InflationKpp : public BaseKnotPointPlacement
    {
    public:
        static std::string getName();

        static InflationKppPtr createInstance(const LTQuant::GenericDataPtr);

        virtual bool createKnotPoints(const CalibrationInstruments& instruments,
                                      const BaseModelPtr baseModel);  
    
    };  //  InflationKpp

	// InflationOnLastPaymentDateKpp
	class InflationOnLastPaymentDateKpp : public BaseKnotPointPlacement
	{
	public:
		static std::string getName();

		static InflationOnLastPaymentDateKppPtr createInstance(const LTQuant::GenericDataPtr);

		virtual bool createKnotPoints(const CalibrationInstruments& instruments,
									const BaseModelPtr baseModel);  

	};  //  InflationOnLastPaymentDateKpp

   
}
#endif //__LIBRARY_PRICERS_FLEXYCF_INFLATIONKPP_H_INCLUDED