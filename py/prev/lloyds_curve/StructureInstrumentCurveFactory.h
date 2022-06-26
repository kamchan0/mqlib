/*****************************************************************************
    StructureInstrumentCurveFactory

	StructureInstrumentCurveFactory is a factory that creates spine structure
	instrument curve related to a structure instrument

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __StructureInstrumentCurveFactory_H__
#define __StructureInstrumentCurveFactory_H__


//	LTQuantLib
#include "LTQuantInitial.h"

//	LTQC
#include "Macros.h"
#include "LT/Ptr.h"

//	FlexYCF
#include "FlexYcfUtils.h"
#include "IStructureInstrumentCurve.h"

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
	typedef std::tr1::function<void
							(const LT::date,
							 const LTQuant::GenericData&,
							 StructureInstrumentCurveList&)	>	StructureInstrumentCurveCreationFunction;
	

	class StructureInstrumentCurveFactory: public DevCore::NameFactory<	StructureInstrumentCurveCreationFunction,
																		StructureInstrumentCurveFactory,
																		StructureInstrumentCurveCreationFunction >,
										   protected DevCore::NonCopyable

	{
	public:
		//	Loads the structure instrument curves for one
		//	instrument type into the vector provided
		static void loadStructureInstrumentCurves(const std::string& instrumentTypeName,
												  const LT::date buildDate,
												  const LTQuant::GenericData& instrumentTypeData,
												  StructureInstrumentCurveList& siCurves);
	
	private:
		friend class DevCore::NameFactory<	StructureInstrumentCurveCreationFunction,
											StructureInstrumentCurveFactory,
											StructureInstrumentCurveCreationFunction >;

		explicit StructureInstrumentCurveFactory();

		static StructureInstrumentCurveFactory* const instance();

		template<class T>
		static void registerStructureInstrumentCurve()
		{
			//	Register the creation funcion against each IDeA alias 
			//	the specified structure instrument type
			instance()->registerObject(getKeyName<T>().string(), StructureInstrumentCurve<T>::loadStructureInstrumentCurves);

			StringVector aliases;
			getAliases<T>(aliases);
			
			for(StringVector::const_iterator iter(aliases.begin()); iter != aliases.end(); ++iter)
			{
				instance()->registerObject(iter->string(), StructureInstrumentCurve<T>::loadStructureInstrumentCurves);
			}
		}
	
	};	//	StructureInstrumentCurveFactory

}	//	FlexYCF

#endif	// __StructureInstrumentCurveFactory_H__
