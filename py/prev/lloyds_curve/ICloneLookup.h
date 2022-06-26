/**
 * @file
 * An interface class implemented by classes involved in cloning.
 *
 * Copyright (c) Lloyds Banking Group, all rights reserved (2010).
 *
 * @author Justin Ware
 *
 */

#ifndef IDEA_SRC_FLEXYCF_ICLONELOOKUP_H
#define IDEA_SRC_FLEXYCF_ICLONELOOKUP_H
#pragma once

// LTQuantLib
#include "LTQuantInitial.h"

namespace FlexYCF
{
    // Forward declarations
    class CloneLookup;

    /**
        @brief An interface implemented by classes that are involved with cloning with lookup table.

        Class instances in FlexYCF have a directed graph relationship (c.f. tree). In order that clones correctly reproduce 
        this directed graph relationship, cloning is performed using a lookup to avoid creating multiple clones of an original
        instance.
    */
    class ICloneLookup
    {
    public:
        /**
            @brief Clone this instance.

            The implementation of this function should use the 'CloneLookup' class to create clones of contained instances. This
            will avoid duplicate clones being made. If base classes have contained instances then is best handled using a pseudo
            copy constructor that takes the 'CloneLookup' instance as a parameter. The implementation of this function would then
            call this constructor.

            @param lookup The lookup table to find already clone instances in.
            @return       A clone of this instance.
        */
		virtual std::tr1::shared_ptr<ICloneLookup> cloneWithLookup(CloneLookup& lookup) const = 0;
    };

    DECLARE_SMART_PTRS(ICloneLookup)
}

#endif
