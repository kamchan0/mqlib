/*****************************************************************************

    Todo: - Add header file description
    
    @Originator	
    
    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved

*****************************************************************************/
#ifndef __FLEX_SCHEDULE_UTILS_H__
#define __FLEX_SCHEDULE_UTILS_H__
#pragma once

// IDeA
#include "Configuration.h"
#include "Data/GenericData.h"
// #include "UtilsEnums.h"

#include "src/Enums/BondType.h"

namespace IDeA
{
	IDeA_FWD_DECLARE_SMART_PTRS(SettledBond);
}
namespace FlexYCF
{
	FWD_DECLARE_SMART_PTRS( FixedLeg );
	class GlobalComponentCache;

	struct ILIndexArg
	{
		ILIndexArg(double fwd1Time, double fwd2Time, double wt)
			: forward1Time(fwd1Time), forward2Time(fwd2Time), weight(wt)
		{};
		double forward1Time;
		double forward2Time;
		double weight;
	};
	
	const ILIndexArg getInflationIndexArguments(const LT::date& buildDate, const LT::date& observeDate, const string& timeLag, const string& resetInterp);

	// Create settled bond object for Yield curve instrument functor
	IDeA::SettledBondPtr createSetBondForInstrument(const LT::TablePtr& instrumentTbl, const LTQuant::GenericData& curveParametersTable, const LT::date& buildDate, 
													const LTQuant::GenericDataPtr& extraInfoTable, const IDeA::BondType& bondType = IDeA::BondType::FixedRateBond);

	// Create a vector of settled bond objects from the bond instrument list, to build the FlexYCF curve
	std::vector<IDeA::SettledBondPtr> createSettledBondVec(	const LTQuant::GenericDataPtr& instrumentTable, const LTQuant::GenericDataPtr& masterTable, 
															const IDeA::BondType& bondType = IDeA::BondType::FixedRateBond);

	void setFixedLegCvgPayDates(FixedLegPtr& fixedLeg, const IDeA::SettledBondConstPtr& setBond);

	FixedLegPtr createFixedLegArgFromSetBond(const IDeA::SettledBondPtr& setBond);
	FixedLegPtr createFixedLegArgFromSetBond(const IDeA::SettledBondPtr& setBond, GlobalComponentCache& globalComponentCache);
}
#endif	//	__FLEX_SCHEDULE_UTILS_H__