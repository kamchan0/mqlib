/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

/// \file

#ifndef __LIBRARY_PRICERS_FLEXYCF_BSPLINEDEFININGFUNCTIONS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BSPLINEDEFININGFUNCTIONS_H_INCLUDED

#include "LTQuantInitial.h"

namespace FlexYCF
{
    /// BSplineDefiningFunctions is an abstract class that 
    /// exposes an interface for the so-called "defining 
    /// functions" psi and phi used in the representation 
    /// of a generalized tension spline.
    ///
    /// See B. Kvasov, Methods of Shape-Preserving Spline 
    /// Approximation.
    class BSplineDefiningFunctions
    {
    public:
        explicit BSplineDefiningFunctions(std::vector<double>& knotSequence,
                                          std::vector<double>& tensionParameters):
            m_knotSequence(knotSequence),
            m_tensionParameters(tensionParameters)
        {
        }

        virtual ~BSplineDefiningFunctions() = 0 { }

        /// Computes the value of the index-th psi function at t
        virtual double psi(const int index, const double t) = 0;
        
        /// Computes the gradient of the index-th psi function at t
        virtual double psiDerivative(const int index, const double t) = 0;

        /// Computes the value of the index-th phi function at t
        virtual double phi(const int index, const double t) = 0;
        
        /// Computes the gradient of the index-th psi function at t
        virtual double phiDerivative(const int index, const double t) = 0;

		virtual std::tr1::shared_ptr<BSplineDefiningFunctions> 
        clone(std::vector<double>& knotSequence, std::vector<double>& tensionParameters) const = 0;

    protected:
        inline
        double getKnot(const int index) const
        {   // +2 because of what the additional knots added at the begininng in TensionSpline
            return m_knotSequence[index+2]; 
        }

        inline
        double getStep(const int index) const
        {
            return getKnot(index+1) - getKnot(index);
        }

        inline
        double getTension(const int index) const
        {   // not clear the tension params make sense on the intervals
            // created by the addition of knots for boundary conditions
            return m_tensionParameters[index+1];    
        }

        double m_tmpKnot;
        double m_tmpTension;

    private:
        std::vector<double>& m_knotSequence;
        std::vector<double>& m_tensionParameters;
    };  //  BSplineDefiningFunctions

    DECLARE_SMART_PTRS( BSplineDefiningFunctions )

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_BSPLINEDEFININGFUNCTIONS_H_INCLUDED