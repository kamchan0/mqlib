#include "stdafx.h"
#include "DiscountFactorArguments.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "DateUtils.h"
#include "GlobalComponentCache.h"

using namespace std;


using namespace LTQC;

namespace FlexYCF
{
    DiscountFactorArguments::DiscountFactorArguments(const LT::date valueDate, const LT::date payDate, const LTQC::Currency& ccy, const LT::Str& index)
       : m_ccy(ccy), m_index(index), m_payDate(payDate)
    {
        if(valueDate == payDate)
        {
            m_flowTime = 0.0;
        }
        else
        {
            m_flowTime = ModuleDate::getYearsBetween(valueDate, payDate);
        }
    }
}