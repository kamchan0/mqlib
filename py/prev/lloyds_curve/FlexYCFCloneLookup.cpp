/**
 * @file
 * A utility class to help in the cloning of FlexYCF.
 *
 * Copyright (c) Lloyds Banking Group, all rights reserved (2010).
 *
 * @author Justin Ware
 *
 */

#include "stdafx.h"

// Standard
#include <hash_map>

// IDeA
#include "FlexYCFCloneLookup.h"
#include "NullDeleter.h"

// LTQuantCore
#include "utils/QCException.h"

// LTQuantLib

// SDK
#include <typeinfo.h>

using namespace LTQC;

namespace
{
    using namespace stdext;

    // The status of the clone
    enum Status { CloneIsIncomplete, CloneAllowingNullDeleter, CloneIsComplete };

    /*
        The clone and associated information.
    */
    struct CloneInformation
    {
        Status m_status;
        FlexYCF::ICloneLookupPtr m_clone;

        CloneInformation()
        : m_status(CloneIsIncomplete)
        {
        }
    };
}

class FlexYCF::CloneLookup::Impl
{
public:
    void allowNullDeleter(ICloneLookup const* original, ICloneLookup* incomplete)
    {
        // We're going to allow NullDeleter copies of this clone to be taken. The clone may later switch to a complete clone after which 
        // NullDeleter copies will not be issued.
        InsertResult result;
        result = m_alreadyCloned.insert(ClonedInstances::value_type(original, CloneInformation()));
        CloneInformation& information = (*result.first).second;
        if (!result.second)
        {
            // An entry already existed for this instance, ensure that it's okay for us to change status
            if (information.m_status != CloneIsIncomplete)
                LTQC_THROW(LTQC::SystemQCException, "Attempt to allow NullDeleter for a clone that is not incomplete in the clone lookup");
        }

        // Asking for a clone of original now will return a NullDeleter copy of the incomplete clone
        information.m_status = CloneAllowingNullDeleter;
        information.m_clone = ICloneLookupPtr(incomplete, NullDeleter());
    }

    ICloneLookupPtr get(ICloneLookup const* original, CloneLookup& lookup)
    {
        // A special case is if the original is the empty pointer. Then the clone is an empty pointer too.
        if (original == 0)
            return ICloneLookupPtr();

        // Look for an existing clone or leave an entry in the map to say we are going to clone this 
        // instance.
        InsertResult result;
        result = m_alreadyCloned.insert(ClonedInstances::value_type(original, CloneInformation()));
        if (!result.second)
        {
            // There is a clone but we need to check what its state is
            CloneInformation& information = (*result.first).second;
            switch (information.m_status)
            {
            case CloneIsIncomplete:
                {
                    // Halfway through cloning so can't create a new clone. We've not been told that we can issue a NullDeleter
                    // copy of the clone. This is typically caused by an unexpected cyclic dependency.
                    type_info const& dynamicType = typeid(*original);
                    LTQC_THROW(LTQC::SystemQCException, "Attempt to create another clone of '" << dynamicType.name() << 
                        "' instance while already cloning, cyclic dependency?");
                }
                break;

            case CloneAllowingNullDeleter:
            case CloneIsComplete:
                // This is fine. Either the clone is a normal shared pointer or a NullDeleter.
                return information.m_clone;

            default:
                LTQC_THROW(LTQC::SystemQCException, "Found clone with unknown status, should never happen");
            }
        }

        // There is no clone so create one.
        ICloneLookupPtr clone = original->cloneWithLookup(lookup);

        // The original insert result is probably invalid so search again
        ClonedInstances::iterator iter = m_alreadyCloned.find(original);
        if (iter == m_alreadyCloned.end())
            LTQC_THROW(LTQC::SystemQCException, "Cannot find previously created entry in the clone lookup, should never happen");

        // Finish the cloning process now we have a complete clone. Purposefully overwriting any existing NullDeleter copy.
        CloneInformation& information = (*iter).second;
        information.m_status = CloneIsComplete;
        information.m_clone = clone;
        return clone;
    }

private:
    typedef stdext::hash_map<ICloneLookup const*, CloneInformation> ClonedInstances;
    typedef std::pair<ClonedInstances::iterator, bool> InsertResult;

    //* A lookup for cloned calibration instruments
    ClonedInstances m_alreadyCloned;
};

//* Create internal implementation
FlexYCF::CloneLookup::CloneLookup()
{
    m_impl = new Impl;
}

//* Destroy internal implementation
FlexYCF::CloneLookup::~CloneLookup()
{
    delete m_impl;
}

void FlexYCF::CloneLookup::allowNullDeleter(ICloneLookup const* original, ICloneLookup* incomplete)
{
    m_impl->allowNullDeleter(original, incomplete);
}

FlexYCF::ICloneLookupPtr FlexYCF::CloneLookup::getEx(ICloneLookupConstPtr const& original)
{
    assert(m_impl != 0);
    return m_impl->get(original.get(), *this);
}
