/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_MODELSELECTION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MODELSELECTION_H_INCLUDED

#include <string>
#include <vector>
#include "LTQuantInitial.h"


#include <lt/const_string.h>

namespace LTQuant
{
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}


namespace FlexYCF
{

	class CalibrationInstruments;

	// Returnsm, given and instrument list, the list of allowed models
	void listOfAllowedModelsFromInstrumentList(const CalibrationInstruments& calibrationInstruments,
											   std::vector<std::string>& allowedModelNames);

	// Returns, given a instrument list table, the name of the most appropriate model to usee
	std::string getDefaultModelFromInstrumentListData(const LTQuant::GenericDataPtr& instrumentList);

	// Returns, given a instrument list, the name of the most appropriate model to usee
	std::string getDefaultModelFromInstrumentList(const CalibrationInstruments& calibrationInstruments);
	


	// Given a model name, a curve details and an instrument list table, returns a top level
	//	table containing sensible default model parameters to use as input for FlexYCF
	LTQuant::GenericDataPtr getDefaultParametersFromModelName(const std::string& modelName,
															  const LTQuant::GenericDataPtr curveDetailsData,
															  const LTQuant::GenericDataPtr instrumentListData);


	/*
	template<class D = LT::Str, 
			 class I = LT::Str>
	struct Interf
	{
		template<class T>
		static T value(const LT::TablePtr& table, const D& dict, const I& item)
		{
			return extract(table->at(1, IDeA_PARAM(dict, item)));
		}

		

		template<class T>
		static T value(const LTQuant::GenericDataPtr& data, const std::string& key)
		{
			return data->get<T>(key, 0);
		}
	};

	template<class T>
	static T extractValue(const LT::TablePtr& table, const std::string& key)
	{
		return extract(table->at(1, key));
	}

//	#define EXTRACT_VALUE(__DATA_STRUCTURE, __DICTIONARY_NAME, __ITEM_NAME)	\
	*/

}

#endif	//	__LIBRARY_PRICERS_FLEXYCF_MODELSELECTION_H_INCLUDED