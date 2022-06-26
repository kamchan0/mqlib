/*****************************************************************************

    EquivalentCurveCalculator

	This file contains the declaration of functions that calculate
	equivalent curve.

    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_EQUIVALENTCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EQUIVALENTCURVE_H_INCLUDED

#include "LTQuantInitial.h"


namespace LTQuant
{
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS;
	FWD_DECLARE_SMART_PTRS( ZeroCurve )
}

namespace IDeA
{
	class YieldCurveIF;
}

namespace  FlexYCF
{

	class CalibrationInstruments;
	FWD_DECLARE_SMART_PTRS( BaseModel );
	
	// Given a source curve table that contains all the information to build a FlexYCF model
	//	and a target curve table that must at least have an instrument list table, 
	//	this function returns a table with the market prices of its instruments calculated so 
	//	as to match their model prices as calculated from the source model. 
	//	Other top level tables are copied from the ones in the target curve table, or,
	//	if missing, set to the ones in source curve table
	/**
	  * @param sourceCurve the source yield curve
	  * @param targetCurveTable the specification of curve instruments and curve parameters in input, the equivalent curve in output
	  */
	void calculateEquivalentCurve(const LT::Ptr<const IDeA::YieldCurveIF>& yieldCurve,
								  const LTQuant::GenericDataPtr& targetCurveTable); 
    void calculateEquivalentCurve(const LTQuant::ZeroCurvePtr& zeroCurve,
								  const LTQuant::GenericDataPtr& targetCurveTable); 
	/*
	LTQuant::GenericDataPtr calculateEquivalentCurve(const LTQuant::GenericDataPtr& sourceCurveTable,
													 const LTQuant::GenericDataPtr& targetCurveTable,
													 const LTQuant::PriceSupplierPtr& priceSupplier);
	*/
	// Calculate the instrument rates such they are priced to par
	// in the model provided
	void calculateImplicitInstrumentRates(const BaseModelPtr& model,
										  const CalibrationInstruments& instruments);

}


#endif // __LIBRARY_PRICERS_FLEXYCF_EQUIVALENTCURVE_H_INCLUDED