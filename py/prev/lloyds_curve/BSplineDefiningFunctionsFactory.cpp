#include "stdafx.h"
#include "BSplineDefiningFunctionsFactory.h"
#include "RationalBSplineDefiningFunctions.h"
#include "HyperbolicBSplineDefiningFunctions.h"
#include "ExponentialBSplineDefiningFunctions.h"


using namespace std;

namespace FlexYCF
{
    
    BSplineDefiningFunctionsFactory::BSplineDefiningFunctionsFactory()
    {
        registerBSplineDefiningFunctions<RationalBSplineDefiningFunctions>();
        registerBSplineDefiningFunctions<HyperbolicBSplineDefiningFunctions>();
        registerBSplineDefiningFunctions<ExponentialBSplineDefiningFunctions>();
    }
    
    BSplineDefiningFunctionsFactory* const BSplineDefiningFunctionsFactory::instance()
    {
        static BSplineDefiningFunctionsFactory bSplineDefiningFunctionsFactory;
        return &bSplineDefiningFunctionsFactory;
    }

    BSplineDefiningFunctionsPtr BSplineDefiningFunctionsFactory::createBSplineDefiningFunctions(const string& splineTypeName,
                                                                                                vector<double>& knotSequence, 
                                                                                                vector<double>& tensionParameters)
    {
        return instance()->internalCreate(splineTypeName)(knotSequence, tensionParameters);
    }
}   //  FlexYCF