/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONSPECS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONSPECS_H_INCLUDED
#pragma once



namespace FlexYCF
{
    /// Left extrapolation specification struct
    struct LeftExtrapSpec
    {
        static std::string getName() {   return "Left"; }
    };

    /// Left extrapolation specification struct
    struct RightExtrapSpec 
    {
        static std::string getName() {   return "Right"; }
    };

    
}
#endif //__LIBRARY_PRICERS_FLEXYCF_EXTRAPOLATIONSPECS_H_INCLUDED