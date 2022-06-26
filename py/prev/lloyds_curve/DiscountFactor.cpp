/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "BaseModel.h"
#include "GenericInstrumentComponent.h"
#include "DiscountFactorArguments.h"
#include "Gradient.h"


using namespace LTQC;

namespace FlexYCF
{
    double GenericInstrumentComponent<DiscountFactorArguments>::getValue(BaseModel const& baseModel)
    {
        return baseModel.getDiscountFactor(m_arguments.getFlowTime());
    }

    void GenericInstrumentComponent<DiscountFactorArguments>::accumulateGradient(BaseModel const& baseModel, 
																				 double multiplier,
																				 GradientIterator gradientBegin,
																				 GradientIterator gradientEnd)
    {
        baseModel.accumulateDiscountFactorGradient(m_arguments.getFlowTime(), multiplier, gradientBegin, gradientEnd);
    }

	void GenericInstrumentComponent<DiscountFactorArguments>::accumulateGradient(BaseModel const& baseModel, 
																				 double multiplier,
																				 GradientIterator gradientBegin,
																				 GradientIterator gradientEnd,
																				 const CurveTypeConstPtr& curveType)
    {
        baseModel.accumulateDiscountFactorGradient(m_arguments.getFlowTime(), multiplier, gradientBegin, gradientEnd, curveType);
    }
	
	void GenericInstrumentComponent<DiscountFactorArguments>::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		if( spread )
		{
			double  spreadDf = dfModel.getSpreadDiscountFactor(m_arguments.getFlowTime());
			baseModel.accumulateDiscountFactorGradient(m_arguments.getFlowTime(), multiplier * spreadDf, gradientBegin, gradientEnd);
		}	
	}
    
	void GenericInstrumentComponent<DiscountFactorArguments>::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		baseModel.accumulateDiscountFactorGradient(m_arguments.getFlowTime(), multiplier, gradientBegin, gradientEnd);
	}
	
	void GenericInstrumentComponent<DiscountFactorArguments>::update()
    {
        // Do Nothing 
    }
}   //  FlexYCF