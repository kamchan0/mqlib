/*****************************************************************************
    Todo: - Add header file description


    @Originator

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/

#ifndef __LIBRARY_MODELS_PIECEWISECONSTANTVOL_H_INCLUDED
#define __LIBRARY_MODELS_PIECEWISECONSTANTVOL_H_INCLUDED
#pragma once

#include "Models/InstantaneousVol.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( ICurve )
}

namespace LTQuant
{
    // Represents a piecewise constant instantaneopus voaltility structure
    // where sigma[i](t) = C[i]: constant
    class PiecewiseConstantVol : public InstantaneousVol
    {
    public:
        PiecewiseConstantVol();

        static LTQuant::InstantaneousVolPtr create(const std::vector<double>& tenorStructure);
        static std::string getName();
        
        virtual double getIntegralSigmaSigma(double startTime, double endTime, double ti, double tj) const;
        virtual double getSigma(double time, double ti) const;
        virtual DevCore::Properties getProperties();
        virtual void addToProblem(Problem& problem);
        virtual void onUpdate();
        
    private:
        FlexYCF::ICurvePtr m_constantVols;
    };

    DECLARE_SMART_PTRS( PiecewiseConstantVol )
}

#endif //__LIBRARY_MODELS_PIECEWISECONSTANTVOL_H_INCLUDED
