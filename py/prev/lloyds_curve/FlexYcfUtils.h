/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __FLEX_YCF_UTILS_H__
#define __FLEX_YCF_UTILS_H__
#pragma once


//	Standard
#include <sstream>

//	FlexYCF
#include "NullDeleter.h"

//	LTQC
#include "lt/const_string.h"
#include "QCUtils.h"

//	IDeA
#include "../../interface/DictionaryManager.h"


#define DEFINE_VECTOR( class_name__ )							\
	typedef std::vector<class_name__>	class_name__##Vector;


namespace FlexYCF
{
	typedef std::vector<LT::Str>	StringVector;

	//	Convers the specified object to a boost shared 
	//	pointer refering to it 
	template<class T>
	std::tr1::shared_ptr<T> toSharedPtr(const T& object)
	{
		return std::tr1::shared_ptr<T>(&const_cast<T&>(object), NullDeleter());
	}

	//	Converts the specified object to string
	template<class T>
	static std::string to_string(const T& object)
	{
		std::ostringstream oss;
		oss << object;
		return oss.str();
	}



	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	IDeA integration utilities
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//	Returns an IDeA::DictionaryKey from a given type
	template<class T>
	const IDeA::DictionaryKey& getKey();

	//	Returns the name of an IDeA::Dictionary key from a given type
	template<class T>
	const LT::Str& getKeyName();

	//	Returns the list of IDeA aliases for the specified type
	template<class T>
	void getAliases(StringVector& aliases);

	//	Returns the IDeA key name and the list of IDeA aliases
	//	of the specified type
	template<class T>
	void getNameAndAliases(StringVector& aliases);


	//
	//	Generic Implementation
	//
	template<class T>
	const LT::Str& getKeyName()
	{
		return getKey<T>().getName();	
	}

	template<class T>
	void getAliases(StringVector& aliases)
	{
		aliases.clear();
		LTQC::split(aliases, LT::trimWhitespace(getKey<T>().getAliases()), ",");
	}

	template<class T>
	void getNameAndAliases(StringVector& aliases)
	{
		getAliases<T>(aliases);
		aliases.push_back(getKeyName<T>());
	}

	std::string date_to_string(const LT::date& date_);

	std::ostream& operator<<(std::ostream& out, const LTQC::Matrix& mat);

	//	inspired from the filter in VectorUtility
	template<class T, class P>
	void filter(const std::vector<T>& inputVector, const P& predicate, std::vector<T>& outputVector)
	{
		outputVector.resize(inputVector.size());
		std::vector<T>::iterator it(std::remove_copy_if(inputVector.begin(), inputVector.end(), outputVector.begin(), predicate));
        outputVector.resize(std::distance(outputVector.begin(), it));
	}

	inline double oneBasisPoint()
	{
		static const double oneBasisPoint_(1.e-4);
		return oneBasisPoint_;
	}

	template<class T>
	T round(const double x)
	{
		return static_cast<T>(floor(x + 0.5));
	}

}
#endif	//	__FLEX_YCF_UTILS_H__