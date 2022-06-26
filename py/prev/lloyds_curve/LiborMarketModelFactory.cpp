#include "stdafx.h"
#include "LiborMarketModelFactory.h"
#include "Models/LMMSimple.h"
#include "Models/LMMDisplacedDiffusion.h"
#include "Data/GenericData.h"

using namespace std;
using namespace LTQC;
using namespace LTQuant;

namespace FlexYCF
{
    LTQuant::LiborMarketModelPtr LiborMarketModelFactory::create(const string& liborMarketModelTypeName,
                                                        const LT::date valueDate,
                                                        const vector<LT::date>& tenorStructure,
                                                        const LTQuant::InstantaneousVolPtr& volatility,
                                                        const LTQuant::CorrelationStructurePtr& correlation,
                                                        const LTQuant::GenericDataPtr& lmmTable)
    {
        return instance()->internalCreate(liborMarketModelTypeName)(valueDate,
                                                                    tenorStructure,
                                                                    volatility,
                                                                    correlation,
                                                                    lmmTable);
    }

    LTQuant::LiborMarketModelPtr LiborMarketModelFactory::create(const LTQuant::GenericDataPtr& lmmTable,
                                                        const LT::date valueDate,
                                                        const vector<LT::date>& tenorStructure,
                                                        const LTQuant::InstantaneousVolPtr& volatility,
                                                        const LTQuant::CorrelationStructurePtr& correlation)
    {
        const string defaultLmmModelName(LMMSimple::getName());
        string lmmModelName(defaultLmmModelName);
        if(lmmTable)
        {
            lmmTable->permissive_get<string>("Name", 0, lmmModelName, defaultLmmModelName);
            LT_LOG << "lmmTable exists. lmmModelName is" << lmmModelName << endl;
        }
        return create(lmmModelName, valueDate, tenorStructure, volatility, correlation, lmmTable);
    }
    
    LiborMarketModelFactory* LiborMarketModelFactory::instance()
    {
        static LiborMarketModelFactory theLiborMarketModelFactory;
        return &theLiborMarketModelFactory;
    }

    LiborMarketModelFactory::LiborMarketModelFactory()
    {
        registerLiborMarketModel<LMMSimple>();
        registerLiborMarketModel<LMMDisplacedDiffusion>();
    }

    LiborMarketModelFactory::~LiborMarketModelFactory()
    {
        // do nothing
    }
}