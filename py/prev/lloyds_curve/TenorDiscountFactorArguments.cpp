/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "TenorDiscountFactorArguments.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "DateUtils.h"
#include "GlobalComponentCache.h"
#include "TenorUtils.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;

    TenorDiscountFactorArguments::TenorDiscountFactorArguments(const LT::date valueDate,
                                                               const LT::date payDate,
                                                               const double tenor,
                                                               const LTQC::Currency& ccy,
                                                               const LT::Str& index) :
        m_tenor(tenor),
        m_payDate(payDate),
        m_ccy(ccy),
        m_index(index)
    {
        if(valueDate >= payDate)
        {
            m_flowTime = 0.0;
        }
        else
        {
            m_flowTime = getYearsBetween(valueDate, payDate);
        }
    }

    TenorDiscountFactorArguments::TenorDiscountFactorArguments(const LT::date valueDate,
                                                               const LT::date payDate,
                                                               const string& tenorDescription,
                                                               const LTQC::Currency& ccy,
                                                               const LT::Str& index):
        m_tenor(tenorDescToYears(tenorDescription)),
        m_payDate(payDate),
        m_ccy(ccy),
        m_index(index)

    {
        
        if(valueDate >= payDate)
        {
            m_flowTime = 0.0;
        }
        else
        {
            m_flowTime = getYearsBetween(valueDate, payDate);
        }
    }
}   //  FlexYCF