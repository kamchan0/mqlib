#include "stdafx.h"
#include "Gradient.h"

using namespace std;

namespace FlexYCF
{
    /// Multiplies the gradient by the scalar specified and
    /// replaces its contents by the result
	void multiply(const double scalar, Gradient& gradient)
    {
		transform(gradient.cbegin(), gradient.cend(), gradient.begin(), [scalar] (double d) {return scalar * d;});
    }

    /// Adds two first gradients and put the result in the third one
    void addTo(const Gradient& gradLeft, const Gradient& gradRight, Gradient& gradResult)
    {
        transform(gradLeft.begin(), gradLeft.end(), gradRight.begin(), gradResult.begin(), std::plus<double>());
    }
    
    /// Subtracts second gradient from the first and put the result in the third one
    void subtractTo(const Gradient& gradLeft, const Gradient& gradRight, Gradient& gradResult)
    {
        transform(gradLeft.begin(), gradLeft.end(), gradRight.begin(), gradResult.begin(), std::minus<double>());
    }

    /// Resize the specified gradient at the given size and fills it with zeros 
    void fillWithZeros(Gradient& vect, const size_t size) 
    {
        vect.clear();
        vect.resize(size, 0.0);
    }

}