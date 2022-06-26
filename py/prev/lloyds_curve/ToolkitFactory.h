/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TOOLKITFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TOOLKITFACTORY_H_INCLUDED

#include "LTQuantInitial.h"
#include "GenericToolkit.h"
#include "Data/GenericData.h"

// DevCore
#include "DevCore/NameFactory.h"

/*
namespace LTQuant
{
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}
*/
namespace FlexYCF
{
	FWD_DECLARE_SMART_PTRS( IToolkit )

	typedef std::tr1::function<IToolkitPtr (const LTQuant::GenericDataPtr, const LTQuant::GenericDataPtr)> ToolkitCreationFunction;

	class ToolkitFactory : public DevCore::NameFactory<ToolkitCreationFunction, ToolkitFactory, ToolkitCreationFunction>
	{
	public:
		static IToolkitPtr createToolkit(const std::string& modelName, 
										 const LTQuant::GenericDataPtr& curveDetailsData,
                                         const LTQuant::GenericDataPtr& instrumentListData)
		{
			return ToolkitFactory::create(modelName)(curveDetailsData,instrumentListData);
		}
	
	private:
		friend class DevCore::NameFactory<ToolkitCreationFunction, ToolkitFactory, ToolkitCreationFunction>;

		explicit ToolkitFactory();
		
		static ToolkitFactory* const instance()
		{
			static ToolkitFactory theToolkitFactory;
			return &theToolkitFactory;
		}

		template<class M>
		static IToolkitPtr createGeneric(const LTQuant::GenericDataPtr& curveDetails, const LTQuant::GenericDataPtr& instrumentList)
		{
			return IToolkitPtr(new GenericToolkit<M>(curveDetails, instrumentList));
		}

		template<class M>
		void registerGenericToolkit()
		{
			ToolkitFactory::instance()->registerObject(M::getName(), ToolkitFactory::createGeneric<M>);
		}
	};
}
#endif	//	__LIBRARY_PRICERS_FLEXYCF_TOOLKITFACTORY_H_INCLUDED