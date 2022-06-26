/*****************************************************************************

	MultiTenorStripperKpp


    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_MULTITENORSTRIPPERKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MULTITENORSTRIPPERKPP_H_INCLUDED
#pragma once

#include "MultiCurveKpp.h"
#include "SparseSetGrid.h"
#include "CurveType.h"
#include "utils/EnumBase.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( MultiTenorModel )
    FWD_DECLARE_SMART_PTRS( CalibrationInstrument )


	LTQC_ENUM_DECLARE_BEGIN(TenorSurfacePrecedenceRule)
		CashRates,
		TenorBasisSwaps,
		CashOnBase,
		CashAndTenorBasisSwaps
	LTQC_ENUM_DECLARE_END(TenorSurfacePrecedenceRule)
	

    /// MultiTenorStripperKpp is the knot-point placement 
    /// algorithm for the multi-tenor model that returns stripper like results
    /// when used with futures
    class MultiTenorStripperKpp : public MultiCurveKpp
    {
    public:
        explicit MultiTenorStripperKpp(const LT::date& valueDate,
                                       const double flatInitialRate,
									   const TenorSurfacePrecedenceRule precedenceRule	= TenorSurfacePrecedenceRule::CashRates,
									   const bool multipleOnly							= false);
  
        static std::string getName()
        {
            return "MultiTenorStripperKpp";
        }

        static MultiCurveKppPtr createInstance(const LTQuant::GenericDataPtr masterTable);

        // A function to call before createKnotPoints to remove 
        //  some instruments.
        virtual void selectInstruments(CalibrationInstruments& instruments, 
                                       const BaseModelPtr baseModel);

        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel);
    

    protected:
        void fillSpreadTypes(std::set<CurveTypeConstPtr>& m_spreadTypes, const CalibrationInstruments& instruments);

    private:
        LT::date					m_valueDate;
        //size_t					m_numberOfInputVariables;
        CurveTypeConstPtr			m_baseRate;
        double						m_flatInitialRate;
		LT::date					m_lastFuturesEndDate;
        LT::date					m_lastFRAEndDate;
		TenorSurfacePrecedenceRule	m_precedenceRule;
		bool						m_multipleOnly;
    };  //  MultiTenorStripperKpp

    DECLARE_SMART_PTRS( MultiTenorStripperKpp )
  
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_MULTITENORSTRIPPERKPP_H_INCLUDED