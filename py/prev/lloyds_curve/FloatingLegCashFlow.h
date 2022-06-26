/*****************************************************************************

    FloatingLegCashFlow


	Base class for cash-flows of a floating leg.
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FLOATINGLEGCASHFLOW_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FLOATINGLEGCASHFLOW_H_INCLUDED
#pragma once

//	FlexYCF
#include "InstrumentComponent.h"
#include "IHasRepFlows.h"
#include "FundingRepFlow.h"
#include "IndexRepFlow.h"
#include "ICloneLookup.h"

namespace IDeA
{
    class AssetDomain;
}

namespace FlexYCF
{
    
    /// Represents a cash-flow of a floating leg. It can be either
    /// a DiscountedForwardRate or FixedCashFlow.
    /// This is to handle LIBOR fixing for a swap and similar things.
    class FloatingLegCashFlow : public InstrumentComponent,
                                public ICloneLookup,
                                public IHasRepFlows<IDeA::Funding>,
                                public IHasRepFlows<IDeA::Index>
    {
    public:
        virtual ~FloatingLegCashFlow() = 0 { }

        virtual std::ostream& print(std::ostream& out) const 
        { 
            out << "Floating Leg Cash-Flow" << std::endl;
            return out; 
        }

		virtual double getRate(const BaseModel& model) = 0;

		virtual void fillRepFlows(IDeA::AssetDomainConstPtr ,
                                  const BaseModel& ,
								  const double ,
                                  IDeA::RepFlowsData<IDeA::Funding>& )
		{
		}

		virtual void fillRepFlows(IDeA::AssetDomainConstPtr ,
                                  const BaseModel& ,
								  const double ,
								  IDeA::RepFlowsData<IDeA::Index>& )
		{
		}

    };

    DECLARE_SMART_PTRS( FloatingLegCashFlow )
	namespace
	{
		std::ostream& operator<<(std::ostream& out, const FloatingLegCashFlowPtr floatingLegCashFlow)
		{
			return floatingLegCashFlow->print(out);
		}
	}
}

namespace FlexYCF
{
    class MultiCcyFloatingLegCashFlow : public MultiCcyInstrumentComponent,
                                        public ICloneLookup
    {
    public:
        virtual ~MultiCcyFloatingLegCashFlow() = 0 { }

        virtual std::ostream& print(std::ostream& out) const 
        { 
            out << "Multi Ccy Floating Leg Cash-Flow" << std::endl;
            return out; 
        }

		virtual double getRate(const BaseModel& domModel, const BaseModel& forModel, double fx) = 0;

    };

    DECLARE_SMART_PTRS( MultiCcyFloatingLegCashFlow )
	namespace
	{
		std::ostream& operator<<(std::ostream& out, const MultiCcyFloatingLegCashFlowPtr floatingLegCashFlow)
		{
			return floatingLegCashFlow->print(out);
		}
	}
}
#endif //__LIBRARY_PRICERS_FLEXYCF_FLOATINGLEGCASHFLOW_H_INCLUDED