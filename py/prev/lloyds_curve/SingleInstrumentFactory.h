/*****************************************************************************

	SingleInstrumentFactory
    
	A factory class that creates single calibration instruments.


    @Originator		Nicolas	Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __IDEA_FLEXYCF_SINGLEINSTRUMENTFACTORY_H_INCLUDED
#define __IDEA_FLEXYCF_SINGLEINSTRUMENTFACTORY_H_INCLUDED

#include "LTQuantInitial.h"

// SDK
#include "lt\const_string.h"
#include "lt\ptr.h"

//	FlexYCF
#include "FlexYcfUtils.h"

// DevCore 
#include "DevCore/NameFactory.h"

#include <functional>

namespace LTQuant
{
	class GDTblWrapper;
    typedef GDTblWrapper GenericData;
}
namespace FlexYCF
{
	FWD_DECLARE_SMART_PTRS( CalibrationInstrument );

	typedef std::tr1::function<	CalibrationInstrumentPtr 
								(const LTQuant::GenericData&,
								const LT::Ptr<LT::date>&,
								const LTQuant::GenericData&,
								const LTQuant::GenericDataPtr&)>	InstrumentCreationFunction;

	// A factory class 
	class SingleInstrumentFactory: public DevCore::NameFactory< InstrumentCreationFunction, SingleInstrumentFactory, InstrumentCreationFunction >
	{
	public:
		static CalibrationInstrumentPtr createInstrument(const std::string& instrumentTypeName,
														 const LTQuant::GenericData& instrumentParametersTable,
														 const LT::Ptr<LT::date>& buildDate,
														 const LTQuant::GenericData& curveParametersTable,
														 const LTQuant::GenericDataPtr& extraInfoTable);

	private:
		friend class DevCore::NameFactory< InstrumentCreationFunction, SingleInstrumentFactory, InstrumentCreationFunction >;
		
		explicit SingleInstrumentFactory();
	
		static SingleInstrumentFactory* const SingleInstrumentFactory::instance();
		
		template<class T>
		void registerInstrument()
		{
			//	Register the creation function against all the IDeA aliases for T
			StringVector aliases;
			getNameAndAliases<T>(aliases);
			for(StringVector::const_iterator iter(aliases.begin()); iter != aliases.end(); ++iter)
			{
				SingleInstrumentFactory::instance()->registerObject(iter->string(), T::create);
			}
		}
		
	};
}
#endif	//	__IDEA_FLEXYCF_SINGLEINSTRUMENTFACTORY_H_INCLUDED