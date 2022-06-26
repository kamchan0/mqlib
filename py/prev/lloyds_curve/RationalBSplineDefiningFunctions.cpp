#include "stdafx.h"
#include "RationalBSplineDefiningFunctions.h"

using namespace std;

namespace FlexYCF
{
    RationalBSplineDefiningFunctions::RationalBSplineDefiningFunctions(vector<double>& knotSequence,
                                                               vector<double>& tensionParameters):
        BSplineDefiningFunctions(knotSequence, tensionParameters)
    {
    }

    string RationalBSplineDefiningFunctions::getName()
    {
        return "Rational";
    }

    BSplineDefiningFunctionsPtr RationalBSplineDefiningFunctions::createInstance(vector<double>& knotSequence, 
                                                                                 vector<double>& tensionParameters)
    {
        return  BSplineDefiningFunctionsPtr(new RationalBSplineDefiningFunctions(knotSequence, tensionParameters));
    }
    
    double RationalBSplineDefiningFunctions::psi(const int index, const double t)
    {
        return pow(t - getKnot(index), 3) / (computePsiDenominatorFactor(index, t) * computeCommonFactor(index));
    }

    double RationalBSplineDefiningFunctions::psiDerivative(const int index, const double t)
    {
        return pow(t - getKnot(index), 2) * (3.0 * computePsiDenominatorFactor(index, t) + getTension(index) * (t - getKnot(index))) 
            / (computeCommonFactor(index) * pow(computePsiDenominatorFactor(index, t), 2));
    }

    double RationalBSplineDefiningFunctions::phi(const int index, const double t)
    {
        return pow(getKnot(index+1) - t, 3) / (computePhiDenominatorFactor(index, t) * computeCommonFactor(index));
    }
    
    double RationalBSplineDefiningFunctions::phiDerivative(const int index, const double t)
    {
        // note the "minus" at the beginning
        return -pow(getKnot(index+1) - t, 2) * (3.0 * computePhiDenominatorFactor(index, t) + getTension(index) * (getKnot(index+1) - t))
            / (computeCommonFactor(index) * pow(computePhiDenominatorFactor(index, t), 2));
    }


    double RationalBSplineDefiningFunctions::computePsiDenominatorFactor(const int index, const double t)
    {
        return 1.0 + getTension(index) * (getKnot(index+1) - t);
    }
    
    double RationalBSplineDefiningFunctions::computePhiDenominatorFactor(const int index, const double t)
    {
        return 1.0 + getTension(index) * (t - getKnot(index));
    }

    /**
        @brief Create a clone.

        Clone uses storage of its containing instance rather than the original.

        @param knotSequence      Reference to the copy in the containing instance.
        @param tensionParameters Reference to the copy in the containing instance.
    */
    BSplineDefiningFunctionsPtr 
    RationalBSplineDefiningFunctions::clone(std::vector<double>& knotSequence, std::vector<double>& tensionParameters) const
    {
        return createInstance(knotSequence, tensionParameters);
    }

    double RationalBSplineDefiningFunctions::computeCommonFactor(const int index) const
    {
        // h[i] * (6 + 6*s[i]*h[i] + 2*s[i]*h[i]*h[i]),  where h[i] = t[i+1] - t[i], s[i]: tension param on interval [t[i], t[i+1])
        return 2.0*getStep(index) * (3.0 + getStep(index)*getTension(index) * (3.0+getStep(index))); 
    }

}   //  FlexYCF