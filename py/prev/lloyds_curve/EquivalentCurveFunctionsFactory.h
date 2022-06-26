/*****************************************************************************
	EquivalentCurveFunctionsFactory

	The factory class for instrument-specific function used by the 
	Equivalent Curve calculator.

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EQUIVALENTCURVEFUNCTIONSFACTORY_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EQUIVALENTCURVEFUNCTIONSFACTORY_H_INCLUDED

//	LTQuantLib
#include "LTQuantInitial.h"
#include "LT/Table.h"

//	FlexYCF
#include "FlexYcfUtils.h"

// DevCore
#include "DevCore/NameFactory.h"

#include <functional>

namespace LTQuant	
{	
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS	
}

namespace FlexYCF
{
	FWD_DECLARE_SMART_PTRS( CalibrationInstrument )
	typedef std::vector<CalibrationInstrumentPtr> InstrumentCollection; 

	typedef std::tr1::function< void
							  (const InstrumentCollection&, 
							  LTQuant::GenericData&)	>	FillInstrumentsTableFunction;

	typedef std::tr1::function< void
							  (const LTQuant::GenericData&,
							  LT::Table&)				>	CopyMarketDataFunction;

	typedef std::pair<FillInstrumentsTableFunction, CopyMarketDataFunction> EquivalentCurveFunctions;

	// A dual function factory that given an instrument type name (or one of its IDeA aliases):
	//	1. returns a function that takes a vector of instruments of the same type to 
	//	set the market quotes of the LTQuant::GenericData representing those instruments
	//	2. returns a function that takes an LTQuant::GenericData containing the
	//	market data of one instrument type and the equivalent LT::Table and copy
	//	them from the former to the latter
	class EquivalentCurveFunctionsFactory : public DevCore::NameFactory< EquivalentCurveFunctions,
																		 EquivalentCurveFunctionsFactory,
																		 EquivalentCurveFunctions	>
	{
	public:
		// Fill the table on instruments with the vector of instruments of 
		//	the same instrument type specified
		static void fillInstrumentsTable(const std::string& instrumentTypeName,
										 const InstrumentCollection& instruments,
										 const LTQuant::GenericDataPtr& instrumentsTable);
		
		static void copyMarketData(const std::string& instrumentTypeName,
								   const LTQuant::GenericDataPtr& instrumentsData,
								   const LT::TablePtr& instrumentsTable);

	private:
		friend class DevCore::NameFactory< EquivalentCurveFunctions,
										   EquivalentCurveFunctionsFactory,
										   EquivalentCurveFunctions	>;
		
		static EquivalentCurveFunctionsFactory* const instance();

		template<class T>
		static void fillInstrumentTable(const FlexYCF::InstrumentCollection& instruments,
										LTQuant::GenericData& instrumentTable);
		template<class T>
		static void copyInstrumentTable(const LTQuant::GenericData& instrumentData,
										LT::Table& instrumentTable);
		
		template<class T>
		void registerFunctions();

		template<class T>
		static const IDeA::DictionaryKey& getRateKey();

		template<class T>
		static const IDeA::DictionaryKey& getMaturityKey();
		
		template<class T>
		static void throwMissingRateHeaderError();
		
		EquivalentCurveFunctionsFactory();
	};


	//
	//	Generic Implementation
	//
	template<class T>
	void EquivalentCurveFunctionsFactory::registerFunctions()
	{
		EquivalentCurveFunctionsFactory::instance()->registerObject(getKeyName<T>().string(),
			make_pair(function<void (const FlexYCF::InstrumentCollection&, LTQuant::GenericData&)>(fillInstrumentTable<T>),
					std::function<void (const LTQuant::GenericData&, LT::Table&)>(copyInstrumentTable<T>)));

		//	Register the functions against all the IDeA aliases for T
		StringVector aliases;
		getAliases<T>(aliases);
		for(StringVector::const_iterator iter(aliases.begin()); iter != aliases.end(); ++iter)
		{
			EquivalentCurveFunctionsFactory::instance()->registerObject(iter->string(),
				make_pair(function<void (const FlexYCF::InstrumentCollection&, LTQuant::GenericData&)>(fillInstrumentTable<T>),
						function<void (const LTQuant::GenericData&, LT::Table&)>(copyInstrumentTable<T>)));
		}
	}

	template<class T>
	void EquivalentCurveFunctionsFactory::fillInstrumentTable(const FlexYCF::InstrumentCollection& instruments,
															  LTQuant::GenericData& instrumentTable)
	{
		const size_t nbInstruments(IDeA::numberOfRecords(instrumentTable));

		if(IDeA::keyExists(instrumentTable, getRateKey<T>()))
		{
			if(instruments.size() == nbInstruments)
			{
				for(size_t cnt(0); cnt < instruments.size(); ++cnt)
				{
					IDeA::inject<double>(instrumentTable, getRateKey<T>(), cnt, instruments[cnt]->getRate());
				}
			}
			// case there are some useless rows in an instrument table
			else if(instruments.size() < nbInstruments)
			{
				size_t instrumentCnt(0);
				string description;
				const string empty("");
				for(size_t row(0); row < nbInstruments; ++row)
				{
					IDeA::permissive_extract<std::string>(instrumentTable, getMaturityKey<T>(), row, description, empty);
				
					if(description != empty)
					{
						IDeA::inject<double>(instrumentTable, getRateKey<T>(), row, instruments[instrumentCnt++]->getRate());											
					}
				}
			}
			else
			{
				LT_THROW_ERROR( "The number of CalibrationInstrument objects is not equal to the number of instruments in the table." );
			}
		}
		else
		{
			throwMissingRateHeaderError<T>();
		}
	}

	template<class T>
	void EquivalentCurveFunctionsFactory::copyInstrumentTable(const LTQuant::GenericData& instrumentData,
															  LT::Table& instrumentTable)
	{
		if(IDeA::keyExists(instrumentData, getRateKey<T>()))
		{
			const size_t nbInstruments(IDeA::numberOfRecords(instrumentData));
			if(IDeA::numberOfRecords(instrumentTable) == nbInstruments)
			{
				for(size_t cnt(0); cnt < nbInstruments; ++cnt)
				{
					IDeA::inject<double>(instrumentTable, 
										 getRateKey<T>(),
										 cnt,
										 IDeA::extract<double>(instrumentData, getRateKey<T>(), cnt));
				}
			}
			else
			{
				LT_THROW_ERROR( "The number of row in the two tables is now the same" );
			}
		}
		else
		{
			throwMissingRateHeaderError<T>();
		}
	}


	
	template<class T>
	void EquivalentCurveFunctionsFactory::throwMissingRateHeaderError()
	{
		LT_THROW_ERROR( "The table does not have a header named after the IDeA DictionaryKey \"" 
			<< getRateKey<T>().getName().string() << "\" or one of its aliases."  );
	}
}

#endif	//	__LIBRARY_PRICERS_FLEXYCF_EQUIVALENTCURVEFUNCTIONSFACTORY_H_INCLUDED