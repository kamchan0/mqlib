/*****************************************************************************
    Todo: - Add header file description


    @Originator

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __LIBRARY_MODELS_INSTANTANEOUSVOLFACTORY_H_INCLUDED
#define __LIBRARY_MODELS_INSTANTANEOUSVOLFACTORY_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "Models/InstantaneousVol.h"

// DevCore
#include "DevCore/NameFactory.h"

#include <functional>

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( InstantaneousVol )
}

namespace FlexYCF
{
    typedef std::tr1::function<LTQuant::InstantaneousVolPtr
                             (const std::vector<double>&)> InstantaneousVolCreationFunction;

    class InstantaneousVolFactory : public DevCore::NameFactory< InstantaneousVolCreationFunction,
                                                                 LTQuant::InstantaneousVol,
                                                                 InstantaneousVolCreationFunction >
    {
    public:
        static LTQuant::InstantaneousVolPtr create(const std::string& instantaneousVolTypeName);
        static LTQuant::InstantaneousVolPtr create(const std::string& instantaneousVolTypeName,
                                          const std::vector<double>& tenorStructure);

        static InstantaneousVolFactory* instance();

    protected:
        explicit InstantaneousVolFactory();
        virtual ~InstantaneousVolFactory();

    private:
        template<class IV>
        static void registerInstantaneousVol();
       
    };

    template<class IV>
    void  InstantaneousVolFactory::registerInstantaneousVol()  
    {
        InstantaneousVolFactory::instance()->registerObject(IV::getName(), IV::create);
    }

}
#endif //__LIBRARY_MODELS_INSTANTANEOUSVOLFACTORY_H_INCLUDED