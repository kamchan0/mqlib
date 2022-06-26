#include "stdafx.h"
#include "HyperbolicBSplineDefiningFunctions.h"

using namespace std;


namespace FlexYCF
{

    HyperbolicBSplineDefiningFunctions::HyperbolicBSplineDefiningFunctions(vector<double>& knotSequence,
                                                                           vector<double>& tensionParameters):
        BSplineDefiningFunctions(knotSequence, tensionParameters)
    {
    }

    string HyperbolicBSplineDefiningFunctions::getName()
    {
        return "Hyperbolic";
    }

    BSplineDefiningFunctionsPtr HyperbolicBSplineDefiningFunctions::createInstance(vector<double>& knotSequence, 
                                                                                   vector<double>& tensionParameters)
    {
        // TO DO: pass right set of params
        return BSplineDefiningFunctionsPtr(new HyperbolicBSplineDefiningFunctions(knotSequence, tensionParameters));
    }

    double HyperbolicBSplineDefiningFunctions::psi(const int index, const double t)
    {
        m_tmpKnot = getKnot(index);
        m_tmpTension = getTension(index);
        
        double tmp((sinh(m_tmpTension * (t-m_tmpKnot)) - m_tmpTension * (t-m_tmpKnot)) / computeDenominator(index));
        return tmp;
    }

    double HyperbolicBSplineDefiningFunctions::psiDerivative(const int index, const double t)
    {
        m_tmpKnot = getKnot(index);
        m_tmpTension = getTension(index);

        double tmp(m_tmpTension * (cosh(m_tmpTension * (t - m_tmpKnot)) - 1.0) / computeDenominator(index));
        return tmp;
    }

    double HyperbolicBSplineDefiningFunctions::phi(const int index, const double t)
    {
        m_tmpKnot = getKnot(index + 1);
        m_tmpTension = getTension(index);
    
        double tmp((sinh(m_tmpTension * (m_tmpKnot-t)) - m_tmpTension * (m_tmpKnot-t)) / computeDenominator(index));
        return tmp;
    }

    double HyperbolicBSplineDefiningFunctions::phiDerivative(const int index, const double t)
    {
        m_tmpKnot = getKnot(index + 1);
        m_tmpTension = getTension(index);

        double tmp(m_tmpTension * (1.0 - cosh(m_tmpTension * (m_tmpKnot - t))) / computeDenominator(index));
        return tmp;
    }

    /**
        @brief Create a clone.

        Clone uses storage of its containing instance rather than the original.

        @param knotSequence      Reference to the copy in the containing instance.
        @param tensionParameters Reference to the copy in the containing instance.
    */
    BSplineDefiningFunctionsPtr 
    HyperbolicBSplineDefiningFunctions::clone(std::vector<double>& knotSequence, std::vector<double>& tensionParameters) const
    {
        return createInstance(knotSequence, tensionParameters);
    }

    double HyperbolicBSplineDefiningFunctions::computeDenominator(const int index) const
    {
        double tmp( m_tmpTension * m_tmpTension * sinh(m_tmpTension * getStep(index)));
        return tmp;
    }

}