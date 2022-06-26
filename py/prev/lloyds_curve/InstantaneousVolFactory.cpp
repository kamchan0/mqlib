#include "stdafx.h"
#include "InstantaneousVolFactory.h"
#include "Models/InstantaneousVol.h"
#include "Models/ConstantVol.h"
#include "Models/RebonatoVol.h"
#include "Models/StochasticRebonatoVol.h"
#include "Models/IndexedStochasticRebonatoVol.h"
#include "Models/StochasticInstantaneousVol.h"
#include "PiecewiseConstantVol.h"

using namespace std;
using namespace LTQuant;

namespace FlexYCF
{
    LTQuant::InstantaneousVolPtr InstantaneousVolFactory::create(const string& instantaneousVolTypeName)
    {
        std::vector<double> voidStructure;
        return instance()->internalCreate(instantaneousVolTypeName)(voidStructure);
    }

    LTQuant::InstantaneousVolPtr InstantaneousVolFactory::create(const string& instantaneousVolTypeName,
                                                        const std::vector<double>& tenorStructure)
    {
        return instance()->internalCreate(instantaneousVolTypeName)(tenorStructure);
    }

    InstantaneousVolFactory* InstantaneousVolFactory::instance()
    {
        static InstantaneousVolFactory theInstantaneousVolFactory;
        return &theInstantaneousVolFactory;
    }

    InstantaneousVolFactory::InstantaneousVolFactory()
    {
        registerInstantaneousVol<ConstantVol>();
        registerInstantaneousVol<RebonatoVol>();
        registerInstantaneousVol<PiecewiseConstantVol>();
    }

    InstantaneousVolFactory::~InstantaneousVolFactory()
    {
        // do nothing
    }
}