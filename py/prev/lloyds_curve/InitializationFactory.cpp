/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InitializationFactory.h"
#include "BreakoutInitialization.h"
#include "ConstantInitialization.h"
#include "InflationInitialization.h"
#include "FromInstrumentsInitialization.h"
#include "CompositeInitialization.h"

using namespace LTQC;

namespace FlexYCF
{
	BaseInitializationPtr InitializationFactory::createInitialization(const std::string& initializationName, 
																	  const LTQuant::GenericDataPtr& initializationParametersTable)
	{
		return InitializationFactory::create(initializationName)(initializationParametersTable);
	}

    InitializationFactory* const InitializationFactory::instance()
	{
		static InitializationFactory initializationFactory;
		return & initializationFactory;
	}

	InitializationFactory::InitializationFactory()
	{
		registerInitialization<BreakoutInitialization>();
		registerInitialization<ConstantInitialization>();
		registerInitialization<InflationInitialization>();
		registerInitialization<FromInstrumentsInitialization>();
		registerInitialization<CompositeInitialization>();
	}

}