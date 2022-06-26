/*****************************************************************************
    CachedDerivInstrument

    Represents a curve instrument with PV, BPV and RateDerivative cached
    so they can be used in the jacobian transformations later


    @Originator		Mark Ayzenshteyn

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

#include "CachedDerivInstrument.h"

using namespace LTQC;

namespace FlexYCF
{
 
    double CachedDerivInstrument::getBPV() const
    {
        return m_cachedValues[CachedDerivInstrument::BPV]();
    }
    double CachedDerivInstrument::getRateDerivative() const
    {
        return m_cachedValues[CachedDerivInstrument::RateDeriv]();
    }
    void CachedDerivInstrument::setBPV(const double& d)
    {
          m_cachedValues[CachedDerivInstrument::BPV](d);
    }
    void CachedDerivInstrument::setRateDerivative(const double& d)
    {
         m_cachedValues[CachedDerivInstrument::RateDeriv](d);
    }

} //namespace flexYCF