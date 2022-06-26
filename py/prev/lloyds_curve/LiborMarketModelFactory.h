/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_LIBORMARKETMODELFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_LIBORMARKETMODELFACTORY_H_INCLUDED
#pragma once

// LTQuantLib
#include "LTQuantInitial.h"

// DevCore
#include "DevCore/NameFactory.h"

#include <functional>

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( LiborMarketModel )
    FWD_DECLARE_SMART_PTRS( InstantaneousVol )
    FWD_DECLARE_SMART_PTRS( CorrelationStructure )
}


namespace FlexYCF
{
    typedef std::tr1::function< LTQuant::LiborMarketModelPtr
                              (const LT::date,               //  valueDate
                              const std::vector<LT::date>&,      // tenorStructure
                              const LTQuant::InstantaneousVolPtr&,
                              const LTQuant::CorrelationStructurePtr&,
                              const LTQuant::GenericDataPtr&) > LiborMarketModelCreationFunction;

    class LiborMarketModelFactory : public DevCore::NameFactory< LiborMarketModelCreationFunction,
                                                                 LiborMarketModelFactory,
                                                                 LiborMarketModelCreationFunction >
    {
    public:
        static LTQuant::LiborMarketModelPtr create(const std::string& liborMarketModelTypeName,
                                          const LT::date valueDate,
                                          const std::vector<LT::date>& tenorStructure,
                                          const LTQuant::InstantaneousVolPtr& volatility,
                                          const LTQuant::CorrelationStructurePtr& correlation,
                                          const LTQuant::GenericDataPtr& lmmTable);

        static LTQuant::LiborMarketModelPtr create(const LTQuant::GenericDataPtr& lmmTable,
                                          const LT::date valueDate,
                                          const std::vector<LT::date>& tenorStructure,
                                          const LTQuant::InstantaneousVolPtr& volatility,
                                          const LTQuant::CorrelationStructurePtr& correlation);
        
        static LiborMarketModelFactory* instance();

    protected:
        explicit LiborMarketModelFactory();
        ~LiborMarketModelFactory();

    private:
        template<class LMM>
        static void registerLiborMarketModel();
    };

    template<class LMM>
    void LiborMarketModelFactory::registerLiborMarketModel()
    {
        LiborMarketModelFactory::instance()->registerObject(LMM::getName(), LMM::create);
    }
}
#endif // __LIBRARY_PRICERS_FLEXYCF_LIBORMARKETMODELFACTORY_H_INCLUDED