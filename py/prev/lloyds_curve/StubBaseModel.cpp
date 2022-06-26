/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

#include "stdafx.h"
#include "StubBaseModel.h"


using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
    StubBaseModel::StubBaseModel(const double rate) :
        m_rate(rate)
    {
    }
    
    StubBaseModel::~StubBaseModel()
    {
    }

    string StubBaseModel::getName()
    {
        return "Stub";
    }

    BaseModelPtr StubBaseModel::createInstance(const LTQuant::GenericDataPtr data, const FlexYCFZeroCurvePtr parent) 
    {
        return StubBaseModelPtr(new StubBaseModel(0.05));   // this is an example;
    }

    double StubBaseModel::getDiscountFactor(const double flowTime) const
    {
        return exp(-flowTime * m_rate);
    }
    
    /// after calib is finished destory all of the calibration
    /// instruments so they do not take up memory via the CachedInstrument component
    /// Stubbase model does not hold instruments
    void StubBaseModel::finishCalibration()
    {
        SingleCurveModel::finishCalibration();
    }
    double StubBaseModel::getTenorDiscountFactor(const double flowTime, 
                                                 const double /* tenor */) const
    {
        return exp(-flowTime * m_rate);
    }

    /**
        @brief Create a clone.

        Uses a lookup table to ensure the directed graph relationships are maintained.

        @param lookup A lookup of previously created clones.
        @return       A clone of this instance.
    */
    ICloneLookupPtr StubBaseModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new StubBaseModel(*this, lookup));
    }

    /**
        @brief A pseudo copy constructor.

        Although this class is simple, have to ensure base classes reproduce their directed graph relationships correctly.

        @param original The instance to be copied.
        @param lookup   A lookup of previously created clones.
    */
    StubBaseModel::StubBaseModel(StubBaseModel const& original, CloneLookup& lookup) :
        SingleCurveModel(original, lookup),
        m_rate(original.m_rate)
    {
    }
}
