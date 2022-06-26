/*****************************************************************************
    
	Todo: - Add source file description

    @Originator

    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InflationIndex.h"
#include "BaseModel.h"
#include "Gradient.h"

using namespace LTQC;

namespace FlexYCF
{
    double GenericInstrumentComponent<InflationIndexArguments>::getValue(BaseModel const& inflationModel)
    {
		const double weight = m_arguments.getWeight();
 		double index((1.0 - weight) * inflationModel.getDiscountFactor(m_arguments.getForward1Time()));
		if(weight > 0.0) 
		{
			index += (weight * inflationModel.getDiscountFactor(m_arguments.getForward2Time()));
		}
		return index;
    }

    void GenericInstrumentComponent<InflationIndexArguments>::accumulateGradient(BaseModel const& baseModel, 
																				 double multiplier,
																				 GradientIterator gradientBegin,
																				 GradientIterator gradientEnd)
    {
		const double weight = m_arguments.getWeight();
		baseModel.accumulateDiscountFactorGradient(m_arguments.getForward1Time(), (1-weight) * multiplier, gradientBegin, gradientEnd);
		if(weight > 0.0)
			baseModel.accumulateDiscountFactorGradient(m_arguments.getForward2Time(), weight * multiplier, gradientBegin, gradientEnd);
    }

	void GenericInstrumentComponent<InflationIndexArguments>::accumulateGradient(BaseModel const& baseModel, 
																				 double multiplier,
																				 GradientIterator gradientBegin,
																				 GradientIterator gradientEnd,
																				 const CurveTypeConstPtr& curveType)
    {
		const double weight = m_arguments.getWeight();
		baseModel.accumulateDiscountFactorGradient(m_arguments.getForward1Time(), (1-weight) * multiplier, gradientBegin, gradientEnd, curveType);
		if(weight > 0.0)
			baseModel.accumulateDiscountFactorGradient(m_arguments.getForward2Time(), weight * multiplier, gradientBegin, gradientEnd, curveType);
    }
	
	void GenericInstrumentComponent<InflationIndexArguments>::accumulateGradientConstantDiscountFactor(
																BaseModel const& baseModel, BaseModel const& dfModel, 
																double multiplier,
																GradientIterator gradientBegin, 
																GradientIterator gradientEnd,
																bool spread)
	{
		const double weight = m_arguments.getWeight();
		baseModel.accumulateDiscountFactorGradient(m_arguments.getForward1Time(), (1-weight) * multiplier, gradientBegin, gradientEnd);
		if(weight > 0.0)
			baseModel.accumulateDiscountFactorGradient(m_arguments.getForward2Time(), weight * multiplier, gradientBegin, gradientEnd);
	}
    
	void GenericInstrumentComponent<InflationIndexArguments>::accumulateGradientConstantTenorDiscountFactor(
																BaseModel const& baseModel, BaseModel const& dfModel, 
																double multiplier,
																GradientIterator gradientBegin, 
																GradientIterator gradientEnd,
																bool spread)
	{
		const double weight = m_arguments.getWeight();
		baseModel.accumulateDiscountFactorGradient(m_arguments.getForward1Time(), (1-weight) * multiplier, gradientBegin, gradientEnd);
		if(weight > 0.0)
			baseModel.accumulateDiscountFactorGradient(m_arguments.getForward2Time(), weight * multiplier, gradientBegin, gradientEnd);
	}
	
	void GenericInstrumentComponent<InflationIndexArguments>::update()
    {
        // Do Nothing 
    }
}   //  FlexYCF