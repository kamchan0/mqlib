/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TENORUTILS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TENORUTILS_H_INCLUDED
#pragma once

#include "DateUtils.h"

namespace FlexYCF
{
    /// This function extends dateDescToYears in that in also 
    /// accepts "ON" as a parameter and treats it as "1D"
    inline double tenorDescToYears(const string& tenorDescription)
    {
        return ModuleDate::dateDescToYears(tenorDescription == "ON" || tenorDescription == "O/N" || tenorDescription == "TN" || tenorDescription == "T/N" ? "1D" : tenorDescription);
    }

    inline string tenorEquivalency(const string& tenorDescription)
    {
        return tenorDescription == "ON" || tenorDescription == "O/N" || tenorDescription == "TN" || tenorDescription == "T/N" ? "1D" : tenorDescription;
    }
}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_TENORUTILS_H_INCLUDED