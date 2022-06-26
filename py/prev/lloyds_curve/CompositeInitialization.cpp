/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "CompositeInitialization.h"
#include "InitializationFactory.h"
#include "Data/GenericData.h"

#include "DataExtraction.h"


using namespace LTQC;

namespace FlexYCF
{
	std::string CompositeInitialization::getName()
	{
		return "Composite";
	}

	BaseInitializationPtr CompositeInitialization::create(const LTQuant::GenericDataPtr& initializationTable)
	{
		CompositeInitializationPtr compositeInitialization(new CompositeInitialization);

		const std::string empty("");
		std::string initializationName;
		LTQuant::GenericDataPtr childInitializationTable;

		if(initializationTable)
		{
			const size_t nbInitializations(IDeA::numberOfRecords(*initializationTable));
			for(size_t cnt(0); cnt < nbInitializations; ++cnt)
			{
				initializationTable->permissive_get<std::string>("Name", cnt, initializationName, empty);
				initializationTable->permissive_get<LTQuant::GenericDataPtr>("Params", cnt, childInitializationTable);
			
				// If a name is provided, create and add the corresponding initialization,
				//	even if no parameters table is specified
				if(initializationName != empty)
				{
					compositeInitialization->createAndAddChild(initializationName, childInitializationTable);
				}
			}
		}
		else
		{
			// default: do nothing, model-dependent anyway
		}

		return compositeInitialization;
	}

	void CompositeInitialization::doInitialize(BaseModel* const model) const
	{
		// for now, apply all initialization
		for(Children::const_iterator iter(m_children.begin()); iter != m_children.end(); ++iter)
		{
			(*iter)->initialize(model);	// stop it residuals norm less than a given threshold?
		}
	}

	
	void CompositeInitialization::createAndAddChild(const std::string& childInitializationName,
												    const LTQuant::GenericDataPtr& childInitializationTable)
	{
		addChild(InitializationFactory::createInitialization(childInitializationName, childInitializationTable));
	}
}