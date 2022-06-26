/*****************************************************************************
    
	FixedLegAdjusted

	Implementation of the FixedLeg class.

    @Originator	

    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "FixedLegAdjusted.h"
#include "InflationIndex.h"
#include "ScheduleUtils.h"
#include "GlobalComponentCache.h"

using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{
	// specialized to the InflationIndexArguments:
	template<>
	void FixedLegAdjusted<InflationIndexArguments>::doInitializeAdjustmentFlows()
	{
		if(m_adjustmentFlows.empty() && !m_fixedLeg->m_cashFlows.empty())
		{
			for(std::vector<Date>::const_iterator iterPayDate = m_fixedLeg->m_paymentDates.begin(); iterPayDate != m_fixedLeg->m_paymentDates.end(); ++iterPayDate)
			{
				const ILIndexArg ilArg = m_adjustmentArgCreator(m_fixedLeg->arguments().getValueDate(), iterPayDate->getAsLTdate());
				m_adjustmentFlows.push_back(InstrumentComponent::getUseCacheFlag()
											? getGlobalComponentCache()->get(InflationIndex::Arguments(	ilArg.forward1Time, 
																										ilArg.forward2Time,
																										ilArg.weight, 
																										iterPayDate->getAsLTdate()))
											: InflationIndex::create(InflationIndexArguments(	ilArg.forward1Time, 
																								ilArg.forward2Time,
																								ilArg.weight, 
																								iterPayDate->getAsLTdate())));
			}
		}
	}
	
	template<>
	LTQuant::GenericDataPtr FlexYCF::FixedLegAdjusted<InflationIndexArguments>::computeCashFlowPVs( const BaseModel& model )
	{
		initilize();
		GenericDataPtr result =  m_fixedLeg->computeCashFlowPVs(*m_dependentModel);
		result->table->nameSet(getLegTypeName());
		size_t index(0);

		for(const_iterator_adjComponent iter(m_adjustmentFlows.begin()); iter!=m_adjustmentFlows.end(); ++iter, ++index)
			result->set<double>("Inflation Index", index, (*iter)->getValue(model));
		
		return result;
	}

}   //  FlexYCF
