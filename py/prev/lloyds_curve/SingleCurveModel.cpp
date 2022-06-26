/*****************************************************************************

    The FlexYCF single curve model.
    
    @Originator		Justin Ware
    
    Copyright (C) Lloyds Banking Group 2010 All Rights Reserved

*****************************************************************************/

#include "stdafx.h"

// FlexYCF
#include "SingleCurveModel.h"

/**
    @brief Pseudo copy constructor.

    The BaseModel needs to see CloneLookup to clone properly.

    @param original The original instance to be copied.
    @param lookup   A lookup of previously created clones.
*/
FlexYCF::SingleCurveModel::SingleCurveModel(SingleCurveModel const& original, CloneLookup& lookup) : 
    BaseModel(original, lookup)
{
}
