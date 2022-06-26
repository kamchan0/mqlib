/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENTFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENTFACTORY_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "Data/GenericData.h"
#include "Pricers/PriceSupplier.h"

//	FlexYCF
#include "CalibrationInstruments.h"
#include "GlobalComponentCache.h"
#include "FlexYcfUtils.h"

// DevCore
#include "DevCore/NameFactory.h"

#include <functional>


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
    FWD_DECLARE_SMART_PTRS( PriceSupplier )
}

namespace FlexYCF
{
    //  *!* Each calibration instrument creation function now takes the global component cache as an extra-parameter *!*
    typedef std::tr1::function <void
                              (CalibrationInstruments&, 
                              LTQuant::GenericDataPtr, 
                              LTQuant::GenericDataPtr,
                              GlobalComponentCache&,
                              const LTQuant::PriceSupplierPtr)> CalibrationInstrumentCreationFunction;

    typedef std::tr1::function <void
                              (CalibrationInstruments&, 
                              LTQuant::GenericDataPtr, 
                              size_t*)> CalibrationInstrumentUpdateFunction;
    typedef std::pair<CalibrationInstrumentCreationFunction,  CalibrationInstrumentUpdateFunction> CalibrationInstrumentFunctions;

    

    /// CalibrationInstrumentFactory is a factory singleton to create CalibrationInstrument's
    ///
    /// Note: to register a new calibration instrument CI with the factory, three things must be done:
    ///  1. add a static getName() function to the CI class that returns a revelant string for this model.
    ///  2. add a static createInstruments(..) function with the same signature as CalibrationInstrumentCreationFunction above 
    ///      that adds the instruments of this type to the object that groups all of them
    ///  3. add the 'registerInstrument<CI>()' instruction to the CalibrationInstrumentFactory constructor
    class CalibrationInstrumentFactory : public DevCore::NameFactory< CalibrationInstrumentFunctions, 
																	  CalibrationInstrumentFactory,
																	  CalibrationInstrumentFunctions >
    {
    public:
        static void loadInstrumentList(CalibrationInstruments& instruments,
                                       LTQuant::GenericDataPtr data,
                                       GlobalComponentCache& globalComponentCache);
        
        static void loadInstrumentList(CalibrationInstruments& instruments,
                                       LTQuant::GenericDataPtr data,
                                       GlobalComponentCache& globalComponentCache,
                                       const LTQuant::PriceSupplierPtr priceSupplier);

        static void updateInstrumentList(CalibrationInstruments& instruments,
                                         LTQuant::GenericDataPtr data);

		static void updateInstrumentRates(CalibrationInstruments& instruments, LTQuant::GenericDataPtr data);
    private:
		friend class DevCore::NameFactory < CalibrationInstrumentFunctions, 
											CalibrationInstrumentFactory,
											CalibrationInstrumentFunctions >;

		static CalibrationInstrumentFactory* const instance();
        
		explicit CalibrationInstrumentFactory();
		
		template <class CI>
        static void registerInstrument()
        {
			//	Register creation functions against each instrument type name ...
			CalibrationInstrumentFactory::instance()->registerObject(CI::getName(), make_pair(*CI::createInstruments, *CI::updateInstruments));
		
			// ... and against their IDeA aliases
			StringVector aliases;
			getAliases<CI>(aliases);
			
			for(StringVector::const_iterator iter(aliases.begin()); iter != aliases.end(); ++iter)
			{
				CalibrationInstrumentFactory::instance()->registerObject(iter->string(), make_pair(*CI::createInstruments, *CI::updateInstruments));
			}
		}

		template<class T>
		void registerAndIgnore();
    };


	namespace details
	{
		void doNothing1(FlexYCF::CalibrationInstruments& /* instrumentList */, 
					   LTQuant::GenericDataPtr /* instrumentTable */, 
					   LTQuant::GenericDataPtr /* data */, 
					   FlexYCF::GlobalComponentCache&, 
					   const LTQuant::PriceSupplierPtr);

		void doNothing2(FlexYCF::CalibrationInstruments& /* instrumentList */, 
						LTQuant::GenericDataPtr /* instrumentTable */, 
						size_t* /* instrumentIndex */);

		FlexYCF::CalibrationInstrumentFunctions doNothingFunctions();
	}

	template<class T>
	void CalibrationInstrumentFactory::registerAndIgnore()
	{
		registerObject(getKeyName<T>().string(), details::doNothingFunctions());
	
		StringVector aliases;
		getAliases<T>(aliases);
		
		for(StringVector::const_iterator iter(aliases.begin()); iter != aliases.end(); ++iter)
		{
			registerObject(iter->string(), details::doNothingFunctions());
		}
	}
}
#endif //__LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENTFACTORY_H_INCLUDED