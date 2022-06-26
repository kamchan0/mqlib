/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_NULLDELETER_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_NULLDELETER_H_INCLUDED
#pragma once

namespace FlexYCF
{
    /// NullDeleter is a pointer deleter that does nothing, 
    /// in order to prevent certain shared_ptr to delete 
    /// their pointer when it should not be deleted.
    struct NullDeleter
    {
    public:
        void operator()(void const *) const
        {
            // Do nothing
        }

    };  // NullDeleter

}   // FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_NULLDELETER_H_INCLUDED