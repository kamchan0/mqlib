/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "BaseModel.h"
#include "GenericInstrumentComponent.h"
#include "TenorDiscountFactorArguments.h"
#include "Gradient.h"


using namespace LTQC;

namespace FlexYCF
{
    double GenericInstrumentComponent<TenorDiscountFactorArguments>::getValue(BaseModel const& baseModel)
    {
		if(!m_arguments.getCurrency().empty() && !m_arguments.getIndex().empty())
			 return baseModel.getTenorDiscountFactor(m_arguments.getFlowTime(), m_arguments.getTenor(),m_arguments.getCurrency(),m_arguments.getIndex());
		else
			return baseModel.getTenorDiscountFactor(m_arguments.getFlowTime(), m_arguments.getTenor());
    }

    void GenericInstrumentComponent<TenorDiscountFactorArguments>::accumulateGradient(BaseModel const& baseModel,
																					  double multiplier,
																					  GradientIterator gradientBegin,
																					  GradientIterator gradientEnd)
    {
        baseModel.accumulateTenorDiscountFactorGradient(m_arguments.getFlowTime(), m_arguments.getTenor(), multiplier, gradientBegin, gradientEnd);
    }

	void GenericInstrumentComponent<TenorDiscountFactorArguments>::accumulateGradient(BaseModel const& baseModel, 
																					  double multiplier,
																					  GradientIterator gradientBegin,
																					  GradientIterator gradientEnd,
																					  const CurveTypeConstPtr& curveType)
    {
        baseModel.accumulateTenorDiscountFactorGradient(m_arguments.getFlowTime(),
				 								        m_arguments.getTenor(),
														multiplier,
														gradientBegin, 
														gradientEnd, 
														curveType);
    }
	
	void GenericInstrumentComponent<TenorDiscountFactorArguments>::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		USE(baseModel)
		USE(dfModel)
		USE(multiplier)
		USE(gradientBegin)
		USE(gradientEnd)
		USE(spread)
		LTQC_THROW(IDeA::SystemException, "accumulateGradientConstantDiscountFactor not implemented");
	}
    
	void GenericInstrumentComponent<TenorDiscountFactorArguments>::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		if(spread)
		{
			double spreadDf = dfModel.getSpreadTenorDiscountFactor(m_arguments.getFlowTime(),m_arguments.getTenor());
			baseModel.accumulateTenorDiscountFactorGradient(m_arguments.getFlowTime(), m_arguments.getTenor(), multiplier * spreadDf, gradientBegin, gradientEnd);
		}
	}
	
	void GenericInstrumentComponent<TenorDiscountFactorArguments>::update()
    {
        // Do Nothing
    }
}