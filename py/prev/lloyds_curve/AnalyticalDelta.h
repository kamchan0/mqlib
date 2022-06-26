/*****************************************************************************

    AnalyticalDelta
    
	This file contains the declaration of function to calculate
	analytical deltas relative to risk instruments


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_ANALYTICALDELTA_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_ANALYTICALDELTA_H_INCLUDED
#pragma once

//	LTQuantLib
#include "LT/Table.h"
#include "PricerMgr.h"

//	FlexYCF
#include "ReplicatingFlows.h"
#include "FundingRepFlow.h"
#include "IndexRepFlow.h"
#include "InflationRepFlow.h"
#include "InstrumentDelta.h"
#include "AssetDomain.h"

namespace LTQuant
{
	class GDTblWrapper;
    typedef GDTblWrapper GenericData;
}

namespace FlexYCF
{
	class BaseModel;

	void calculateIRAnalyticalDelta(const BaseModel& model,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
								  InstrumentDeltaVector& deltaVector);
	
	void calculateIRAnalyticalDelta(IDeA::AssetDomainConstPtr modelAssetDomain,
								    IDeA::AssetDomainConstPtr childModelAssetDomain,
								  const BaseModel& model,
								  const BaseModel& childModel,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
								  InstrumentDeltaVector& deltaVector,
								  double fx);
    void calculateIRAnalyticalDelta(IDeA::AssetDomainConstPtr modelAssetDomain,
								    IDeA::AssetDomainConstPtr childModelAssetDomain,
								  const BaseModel& model,
								  const BaseModel& childModel,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlowsChild,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlowsChild,
								  InstrumentDeltaVector& deltaVector,
								  double fx);
	void calculateIRAnalyticalDelta(IDeA::AssetDomainConstPtr modelAssetDomain,
									IDeA::AssetDomainConstPtr childModelAssetDomain,
									IDeA::AssetDomainConstPtr childChildModelAssetDomain,
								  const BaseModel& model,
								  const BaseModel& childModel,
								   const BaseModel& childChildModel,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
								  InstrumentDeltaVector& deltaVector,
								  double fx);

	void calculateILAnalyticalDelta(const BaseModel& model,
								  const IDeA::ReplicatingFlows<IDeA::Inflation>& inflationRepFlows,
								  InstrumentDeltaVector& deltaVector);

	void calculateStructureAnalyticalDelta(const BaseModel& model,
										   const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
										   const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
										   InstrumentDeltaVector& deltaVector);

	FlexYCF::InstrumentDeltaVector calculateRisk(FlexYCF::BaseModelPtr model, IDeA::AssetDomainConstPtr assetDomain, std::vector<IDeA::YieldCurveIFConstPtr> constChildYC, std::vector<LT::Str> childModelAD, IDeA::RepFlowsData<IDeA::Funding> fundingRepFlows, IDeA::RepFlowsData<IDeA::Index> indexRepFlows,  const std::map<LT::Str, double>& fxRates);
}

#endif	//	__LIBRARY_PRICERS_FLEXYCF_ANALYTICALDELTA_H_INCLUDED