/*****************************************************************************

    @Originator	
    
    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEFIXEDKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEFIXEDKPP_H_INCLUDED
#pragma once

#include "SingleCurveKpp.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( SingleCurveFixedKpp )

    class SingleCurveFixedKpp : public SingleCurveKpp
    {
    public:
        static std::string getName();

		SingleCurveFixedKpp(const LTQuant::GenericDataPtr& fixedKpTbl, const BaseKnotPointPlacementPtr& nestedKpp);

        static SingleCurveFixedKppPtr createInstance(const LTQuant::GenericDataPtr masterTable);

		virtual void selectInstruments(CalibrationInstruments& instruments, 
                                       const BaseModelPtr baseModel);
        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel);
	private:

		LTQuant::GenericDataPtr m_fixedKpTbl;
		BaseKnotPointPlacementPtr m_nestedKpp;

    };   //  SingleCurveFixedKpp

	DECLARE_SMART_PTRS( SingleCurveFixedKpp )

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_SINGLECURVEFIXEDKPP_H_INCLUDED