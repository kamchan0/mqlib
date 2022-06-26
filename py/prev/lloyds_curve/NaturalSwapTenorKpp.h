/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_NATURALSWAPTENORKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_NATURALSWAPTENORKPP_H_INCLUDED
#pragma once

#include "MultiCurveKpp.h"


namespace LTQuant
{
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS;
}

namespace FlexYCF
{
	class CalibrationInstruments;
	FWD_DECLARE_SMART_PTRS( BaseModel );
	FWD_DECLARE_SMART_PTRS( CurveType );

	// Places cash, futures and swaps on the curve whose Tenor
	// is the natural Tenor of the floating legs of the swaps
	class NaturalSwapTenorKpp : public MultiCurveKpp
	{
	public:
		explicit NaturalSwapTenorKpp(const LT::date valueDate,
									 const CurveTypeConstPtr& naturalSwapTenor);
		
		static std::string getName();
		static MultiCurveKppPtr createInstance(const LTQuant::GenericDataPtr& masterTable);

		virtual void selectInstruments(CalibrationInstruments& instruments,
									   const BaseModelPtr model);
		virtual bool createKnotPoints(const CalibrationInstruments& instruments,
									  const BaseModelPtr model);

	private:
		LT::date m_valueDate;
		CurveTypeConstPtr m_naturalSwapTenor;
	};
}


#endif	//	__LIBRARY_PRICERS_FLEXYCF_NATURALSWAPTENORKPP_H_INCLUDED