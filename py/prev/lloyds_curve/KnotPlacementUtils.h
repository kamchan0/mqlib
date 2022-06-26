/*****************************************************************************

	KnotPlacementUtils

	This file contains utility functions that are used by several knot-point 
	placement

    
    @Originator		Nicolas	Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __KnotPlacementUtils_H__
#define __KnotPlacementUtils_H__


//	LTQuantLib
#include "LTQuantInitial.h"
#include "ModuleDate/InternalInterface/Utils.h"
#include "utils/EnumBase.h"

//	Standard
#include <set>
#include <string>


namespace FlexYCF
{
	class BaseModel;
	FWD_DECLARE_SMART_PTRS( CalibrationInstrument )
	FWD_DECLARE_SMART_PTRS( CurveType )

	class CalibrationInstruments;

    LTQC_ENUM_DECLARE_BEGIN(CashSelection)
	    BaseRate,
		FutureStartBlending,
        FutureStartLinear
	LTQC_ENUM_DECLARE_END(CashSelection)

	// Function used to remove cash rates that are not of one of the required tenors
	bool cashAndNotOneOfRequiredTenors(const CalibrationInstrumentPtr& inst, 
									   const std::set<CurveTypeConstPtr>& spreadTypes,
									   const CurveTypeConstPtr& baseRate);

	// Function used to remove all cash rates except the one of the specified Tenor
	bool cashAndNotBaseRate(const CalibrationInstrumentPtr& instrument,
							const CurveTypeConstPtr& baseRate);

    bool cashAndAfterGivenDate(const CalibrationInstrumentPtr& instrument, const LT::date& date);

    // Function used to remove all FRAs that are not of a specific base rate Tenor
    bool fraAndNotBaseRate( const CalibrationInstrumentPtr& instrument,
							const CurveTypeConstPtr& baseRate);
    
    void removeCashRatesAfterGivenDate(const LT::date& date, CalibrationInstruments& instruments);
	
    //	Remove those cash instruments whose Tenor does not match the base rate
	void removeNonBaseCashRates(const CurveTypeConstPtr& baseRate, CalibrationInstruments& instruments);
	
	// Function used to remove interest rate swaps whose end date is before the last futures end date
    bool swapAndLessThanFuturesEndDate(const CalibrationInstrumentPtr& inst, const LT::date& lastFuturesEndDate);
	
	bool OISBasisSwapAndLessThanOISEndDate(const CalibrationInstrumentPtr& inst, const LT::date& OISEndDate);
	
	//	Helper functor used to determine whether the specified instrument is a cash, futures or vanilla swap
	struct IsCashFuturesFRASwap: public std::unary_function<CalibrationInstrumentPtr, bool>
	{
		bool operator()(const CalibrationInstrumentPtr& instrument) const;
	};

	//	Removes instruments that are neither cash rate, futures or vanilla swaps
	void removeNonCashFuturesSwapOrStructure(CalibrationInstruments& instruments);

	// Helper function used to remove futures whose fixing date is less than the build date
	bool isFutureAndFixesBeforeBuildDate(const CalibrationInstrumentPtr& instrument,
										 const LT::date& buildDate);

	// Remove those futures whose fixing date is before the build date from the specified instruments
	void removeFixedFutures(const LT::date& valueDate, CalibrationInstruments& instruments);

    // Remove FRAs
	void removeFRAs(CalibrationInstruments& instruments);

    // Remove FRAs that are not on the base rate
    void removeNonBaseRateFRAs(const CurveTypeConstPtr& baseRate, CalibrationInstruments& instruments);

    /**
    * Remove tenor basis swaps without basis, that is the ones where the reference and other legs have the same tenor
    * @param[out] instruments as output the filtered instruments
    */
    void removeTenorBasisSwapsWithoutBasis(CalibrationInstruments& instruments);

    // Returns whether the specified instrument is a tenor basis swap of the specified maturity
    bool tenorBasisOfGivenMaturity(const CalibrationInstrumentPtr& instrument, const string& maturity);

	// Returns whether the specified instrument is "synthetic" a tenor basis swap,
	//	in the sense that is maturity is less its non-base tenor
	bool tenorBasisAndMaturityLessThanOtherLegTenor(const CalibrationInstrumentPtr& instrument);

	bool tenorBasisAndMaturityNotAMultipleOfOtherLegTenor(const CalibrationInstrumentPtr& instrument);

    /**
    * Check if a given calibration instrument is a tenor basis swap with identical tenors on both legs
    * @param[in] instrument the calibration instrument
    */
    bool tenorBasisWithoutBasis(const CalibrationInstrumentPtr& instrument);
    
	//	Returns the end date of the last futures
	LT::date getLastFuturesEndDate(const LT::date& valueDate, const CalibrationInstruments& instruments);

	//	Returns the start date of the first futures
    LT::date getFirstFuturesStartDate(const LT::date& valueDate, const CalibrationInstruments& instruments);

    // Returns the start date of the first FRA
    LT::date getFirstFRAStartDate(const LT::date& valueDate, const CalibrationInstruments& instruments);

    // Returns the end date of the last FRA
	LT::date getLastFRAEndDate(const LT::date& valueDate, const CalibrationInstruments& instruments);
     
	LT::date getLastOISEndDate(const LT::date& valueDate, const CalibrationInstruments& instruments);
    
	bool getFirstCashEndDateOnOrAfterGivenDate(const LT::date& date, const CalibrationInstruments& instruments, LT::date& cashEndDate);
	
    //	Returns whether there exists a cash instrument with the specified Tenor
	//	in the calibration instruments
	bool cashInstrumentOfGivenTenorExists(const CalibrationInstruments& instruments,
										  const CurveTypeConstPtr& tenor);

	//	Select instrument for the Stripper model
	void selectInstrumentsForStripperModel(const BaseModel& model, CalibrationInstruments& instruments, CashSelection cashSelection = CashSelection::BaseRate );

    void selectCashInstrumentsForStripperModel(const BaseModel& model, CalibrationInstruments& instruments, CashSelection cashSelection);

    double interpolatedZeroRateFromCash(const BaseModel& model, const CalibrationInstruments& instruments, const LT::date& inputDay);

	void removeDuplicateCashInstruments(CalibrationInstruments& instruments);
	void removeDuplicateCrossCurrencySwaps(CalibrationInstruments& instruments);
	void removeDuplicateTenorBasisSwaps(CalibrationInstruments& instruments, size_t numberOfDatesToTest);
}
#endif	//	__KnotPlacementUtils_H__

