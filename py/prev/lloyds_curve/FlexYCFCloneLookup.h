/**
 * @file
 * A utility class to help in the cloning of FlexYCF.
 *
 * Copyright (c) Lloyds Banking Group, all rights reserved (2010).
 *
 * @author Justin Ware
 *
 */

#ifndef IDEA_SRC_FLEXYCF_FLEXYCFCLONELOOKUP_H
#define IDEA_SRC_FLEXYCF_FLEXYCFCLONELOOKUP_H
#pragma once

// FlexYCF
#include "ICloneLookup.h"

namespace FlexYCF
{
    /**
        @brief A utility class used in the cloning of FlexYCF.

        The aim of this class is to help in maintaining the directed graph relationships that exist inside FlexYCF e.g.
        CalibrationInstrument instances are referred to from many locations. Simplistic cloning will flatten all these relationships.
    */
    class CloneLookup
    {
    private:
        class Impl;

    public:
        CloneLookup();
        ~CloneLookup();

        /**
            @brief Get a clone of the object.

            If the object has already been cloned then the existing clone is returned.

            @param original The original instance.
            @return         The corresponding clone.
        */
        template <class T>
		std::tr1::shared_ptr<T> get(std::tr1::shared_ptr<T> const& original)
        {
			return std::tr1::dynamic_pointer_cast<T>(getEx(original));
        }

        /**
            @brief Allow the lookup to give out pointers to this instance using NullDeleter.

            There is an issue when cloning instances where contained instances refer back to their containing instance using a
            NullDeleter. The lookup will only know about the clone once it has been created but that is too late as the contained
            instance will have a clone of the containing instance. This function allows a clone to be registered with the lookup table
            before it is completed. Any attempt to create this clone again will have a NullDeleter copy of the clone being created.
            Once the clone is created then it will no longer return NullDeleter copy.

            @param original   The original instance.
            @param incomplete The incomplete clone.
        */
        void allowNullDeleter(ICloneLookup const*, ICloneLookup* incomplete);

    private:
        //* Internal representation
        Impl* m_impl;

        ICloneLookupPtr getEx(ICloneLookupConstPtr const& original);

        CloneLookup(CloneLookup const&); // no implementation
        CloneLookup& operator=(CloneLookup const&); // no implementation
    };

    namespace CloneLookupUtils
    {
        /**
            @brief Assign from one vector to another vector, cloning elements.

            A lookup is used to ensure that directed graph relationships are maintained.

            @param from   The original instances are in this vector.
            @param to     The clones are assigned to this vector (ovewriting the original contents).
            @param lookup The lookup of previously created clones.
        */
        template <class T>
        void assign(std::vector<T> const& from, std::vector<T>& to, CloneLookup& lookup)
        {
            to.clear();
            to.reserve(from.size());
            std::vector<T>::const_iterator it;
            for (it = from.begin(); it != from.end(); ++it)
                to.push_back(lookup.get(*it));
        }

    }
}

#endif
