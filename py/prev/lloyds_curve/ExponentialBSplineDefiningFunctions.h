/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EXPONENTIALBSPLINEDEFININGFUNCTIONS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EXPONENTIALBSPLINEDEFININGFUNCTIONS_H_INCLUDED
#pragma once

#include "BSplineDefiningFunctions.h"


namespace FlexYCF
{
    /// Represents the defining functions that
    /// generate exponential tension splines.
    class ExponentialBSplineDefiningFunctions : public BSplineDefiningFunctions
    {
    public:
        ExponentialBSplineDefiningFunctions(std::vector<double>& knotSequence,
                                            std::vector<double>& tensionParameters);

        static std::string getName();
        static BSplineDefiningFunctionsPtr createInstance(std::vector<double>& knotSequence, 
                                                          std::vector<double>& tensionParameters);

        virtual double psi(const int index, const double t);
        virtual double psiDerivative(const int index, const double t);

        virtual double phi(const int index, const double t);
        virtual double phiDerivative(const int index, const double t);

        virtual BSplineDefiningFunctionsPtr clone(std::vector<double>& knotSequence, std::vector<double>& tensionParameters) const;

    private:
        double computeDenominator(const int index);
    };  //  ExponentialBSplineDefiningFunctions

}

#endif //__LIBRARY_PRICERS_FLEXYCF_EXPONENTIALBSPLINEDEFININGFUNCTIONS_H_INCLUDED