/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INFLATIONTOOLKIT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INFLATIONTOOLKIT_H_INCLUDED

// to be replaced with LT::ptr and LT::table after migration
#include <boost/shared_ptr.hpp>
#include "Data/GenericData.h"

#include "IToolkit.h"



namespace FlexYCF
{
	//template<template <typename> 
	//			 class TSmartPtr		= boost::shared_ptr,
	//			 class TTable			= LTQuant::GenericDataPtr >
	class InflationToolkit : public IToolkit//<TSmartPtr, TTable>
	{
	public:
		/*
		virtual TSmartPtr<BaseModel> createBaseModel(const TTable& masterTable) const;
		virtual TSmartPtr<KnotPointPlacement> createKnotPointPlacement(const TTable& masterTable) const;
		virtual TSmartPtr<BaseInitialization> createInitialization(const TTable& initializationParamsTable) const;
		virtual TSmartPtr<BaseSolver> createSolver(const TTable& solverParamsTable) const;
		*/
	};

	

}
#endif //__LIBRARY_PRICERS_FLEXYCF_INFLATIONTOOLKIT_H_INCLUDED