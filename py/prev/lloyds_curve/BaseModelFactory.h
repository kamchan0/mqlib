/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BASEMODELFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BASEMODELFACTORY_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"

// DevCore
#include "DevCore/NameFactory.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
    FWD_DECLARE_SMART_PTRS(PriceSupplier)
    FWD_DECLARE_SMART_PTRS(FlexYCFZeroCurve)
}


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BaseModel )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )

	typedef std::tr1::function<BaseModelPtr (LTQuant::GenericDataPtr, const LTQuant::FlexYCFZeroCurvePtr)> ModelCreationFunction;
    
    /// Model Factory:
    /// To register a new model M with the factory, three things must be done:
    ///  1. add a static getName() function to the M class that returns a revelant string for this model.
    ///  2. add a static createInstance() function with the same signature as ModelCreationFunction above.
    ///  3. add the 'registerModel<M>()' instruction to the ModelFactory constructor
    class ModelFactory : public DevCore::NameFactory< ModelCreationFunction, ModelFactory, ModelCreationFunction >
    {
    public:
        // Creates a shared_ptr to BaseModel, using the registered 
        //  ModelCreationFunction associated with the modelName
        static BaseModelPtr createModel(const string& modelName, const LTQuant::GenericDataPtr masterTable, const LTQuant::FlexYCFZeroCurvePtr parent );

        // List models - returns a list of models available from this factory
        static void listModels(
                        std::vector<std::string>    &modelNames);
        // Query if given model is a FlexYCF model
        static bool isValidModel(const std::string& modelName);
    private:
		friend class DevCore::NameFactory< ModelCreationFunction, ModelFactory, ModelCreationFunction >;

        static ModelFactory* const instance();

        template<class M>
        static void registerModel()
        {
            ModelFactory::instance()->registerObject(M::getName(), M::createInstance);
        }
        void modelNames(std::vector<std::string>    &modelNames)
        {
            for ( std::map< std::string, ModelCreationFunction>::const_iterator iter = getObjects().begin();
                iter != getObjects().end(); ++iter )
            {
                modelNames.push_back(iter->first);
            }
        }
        explicit ModelFactory();

    };  //  ModelFactory

}   //  FlexYCF


#endif //__LIBRARY_PRICERS_FLEXYCF_BASEMODELFACTORY_H_INCLUDED