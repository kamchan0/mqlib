/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INSTANTANEOUSVOLCALIBRATOR_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INSTANTANEOUSVOLCALIBRATOR_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "ImpliedVolQuotes.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( InstantaneousVol )
}

namespace FlexYCF
{

    /// A class to calibrate instantaneous volatilities
    class InstantaneousVolCalibrator
    {
    public:
        /// Calibrates the instantaneous volatility to the implied
        /// caplet vol quotes.
        /// Returns the sum of squared residual errors.
        double calibrate(const LTQuant::InstantaneousVolPtr& instantaneousVol, 
                         const ImpliedVolQuotes& impliedCapletVolQuotes,
                         const double epsilon = 1.e-3,
                         const long maxFunctionEvals = 10000,
                         const double ftol = 1.0e-14,
                         const double xtol = 1.0e-14,
                         const double gtol = 1.0e-14);

    private:
        double impliedCapletVolDiff(const size_t index);
        void update() const;

        LTQuant::InstantaneousVolPtr m_instantaneousVol;
        const ImpliedVolQuotes * m_impliedVolQuotes;
        double m_expiryTmp;
        double m_capletVolTmp;
    };
}

#endif //__LIBRARY_PRICERS_FLEXYCF_INSTANTANEOUSVOLCALIBRATOR_H_INCLUDED