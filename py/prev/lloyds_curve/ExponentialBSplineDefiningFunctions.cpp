#include "stdafx.h"
#include "ExponentialBSplineDefiningFunctions.h"

using namespace std;

namespace FlexYCF
{
    ExponentialBSplineDefiningFunctions::ExponentialBSplineDefiningFunctions(vector<double>& knotSequence,
                                                                             vector<double>& tensionParameters):
        BSplineDefiningFunctions(knotSequence, tensionParameters)   
    {
    }

    string ExponentialBSplineDefiningFunctions::getName()
    {
        return "Exponential";

    }

    BSplineDefiningFunctionsPtr ExponentialBSplineDefiningFunctions::createInstance(vector<double>& knotSequence, 
                                                                                    vector<double>& tensionParameters)
    {
        return BSplineDefiningFunctionsPtr(new ExponentialBSplineDefiningFunctions(knotSequence, tensionParameters));
    }

    double ExponentialBSplineDefiningFunctions::psi(const int index, const double t)
    {
        return pow(t - getKnot(index), 3) * exp(getTension(index) * (t - getKnot(index+1))) / computeDenominator(index);
    }

    double ExponentialBSplineDefiningFunctions::psiDerivative(const int index, const double t)
    {
        return pow(t - getKnot(index), 2) * exp(getTension(index) * (t - getKnot(index+1))) 
            * (3.0 + getTension(index) * (t - getKnot(index))) / computeDenominator(index);
    }

    double ExponentialBSplineDefiningFunctions::phi(const int index, const double t)
    {
        return pow(getKnot(index+1) - t, 3) * exp(getTension(index) * (getKnot(index) - t)) / computeDenominator(index);
    }

    double ExponentialBSplineDefiningFunctions::phiDerivative(const int index, const double t)
    {
        // Notice the minus in front of the terms
        return -pow(getKnot(index+1) - t, 2) * exp(getTension(index) * (getKnot(index) - t))
            * (3.0 + getTension(index) * (getKnot(index+1) - t)) / computeDenominator(index);
    }

    /**
        @brief Create a clone.

        Clone uses storage of its containing instance rather than the original.

        @param knotSequence      Reference to the copy in the containing instance.
        @param tensionParameters Reference to the copy in the containing instance.
    */
    BSplineDefiningFunctionsPtr 
    ExponentialBSplineDefiningFunctions::clone(std::vector<double>& knotSequence, std::vector<double>& tensionParameters) const
    {
        return createInstance(knotSequence, tensionParameters);
    }

    double ExponentialBSplineDefiningFunctions::computeDenominator(const int index)
    {
        return getStep(index) * (6.0 + getTension(index) * getStep(index) * (6.0 + getStep(index)));
    }

}