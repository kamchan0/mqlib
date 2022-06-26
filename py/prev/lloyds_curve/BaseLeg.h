/*****************************************************************************

    BaseLeg

	The base class for legs.


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __IDEA_FLEXYCF_BASELEG_H_INCLUDED
#define __IDEA_FLEXYCF_BASELEG_H_INCLUDED

//	FlexYCF
#include "IHasCashFlows.h"
#include "BaseModel.h"
#include "IHasRepFlows.h"
#include "IndexRepFlow.h"
#include "FundingRepFlow.h"

//	LTQuantLib
#include "ModuleDate/InternalInterface/ScheduleGenerator.h"
#include "Data\GenericData.h"

namespace IDeA
{
    class AssetDomain;
}

namespace FlexYCF
{
    /**
        @brief The base implementation of floating and fixed legs.

        There is an assumption (implicitly) made here with regard to cloning. The arguments type can be correctly cloned
        using the default copy constructor. That is, there is nothing in the arguments that needs the CloneLookup when
        cloning.
    */
	template<class T>
	class BaseLeg : public IHasCashFlows,
					public IHasRepFlows<IDeA::Funding>,
					public IHasRepFlows<IDeA::Index>
	{
	public:
		explicit BaseLeg(const T& arguments):
			m_arguments(arguments)
		{
		}
		
		virtual LTQuant::GenericDataPtr getCashFlows();
		virtual LTQuant::GenericDataPtr computeCashFlowPVs(const BaseModel& model);

		//	IHasRepFlows<IDeA::Funding> interface
        virtual void fillRepFlows(IDeA::AssetDomainConstPtr ,
                                  const BaseModel& ,
								  const double ,
								  IDeA::RepFlowsData<IDeA::Funding>& )
		{
			//	do nothing by default
		}

		//	IHasRepFlows<IDeA::Index> interface
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr ,
                                  const BaseModel& ,
								  const double ,
								  IDeA::RepFlowsData<IDeA::Index>& )
		{
			//	do nothing by default
		}

        /// remove cash flows (will be re-initisalied on next initializeCashFlowPricing)
        /// to reduce memory footprint
        virtual void cleanupCashFlows() = 0;


	protected:
		inline void initializeCashFlowPricing()
		{
			initializeCashFlows();
			doInitializeCashFlowPricing();
        }

        ModuleDate::Schedule::ScheduleEvents m_scheduleDates;
		std::vector< Date >				  m_paymentDates;
        
		const T& arguments() const
        {
			return m_arguments;
		}

	private:
		virtual std::string getLegTypeName() const	= 0;
		virtual void initializeCashFlows()			= 0;
		virtual void doInitializeCashFlowPricing()	= 0;		
		virtual void fillSingleCashFlowPV(const size_t index,
										  const LT::date startDate,
										  const LT::date endDate,
										  const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable) = 0;

		T m_arguments;
	};

	
	template<class T>
	LTQuant::GenericDataPtr BaseLeg<T>::getCashFlows()
	{
		initializeCashFlows();

		const LTQuant::GenericDataPtr result(new LTQuant::GenericData(getLegTypeName(), 0));
		
		size_t index(0);
        for(ModuleDate::Schedule::ScheduleEvents::const_iterator iter(m_scheduleDates.begin()); iter != m_scheduleDates.end(); ++iter, ++index)
		{
			result->set<LT::date>("Accrual Start Date", index, iter->begin());
			result->set<LT::date>("Accrual End Date", index, iter->end());
		}

		return result;
	}

	template<class T>
	LTQuant::GenericDataPtr BaseLeg<T>::computeCashFlowPVs(const BaseModel& model)
	{
		initializeCashFlowPricing();

		const LTQuant::GenericDataPtr result(new LTQuant::GenericData(getLegTypeName(), 0));
		size_t index(0);

        for(ModuleDate::Schedule::ScheduleEvents::const_iterator iter(m_scheduleDates.begin()); iter != m_scheduleDates.end(); ++iter, ++index)
		{
			result->set<LT::date>("Accrual Start Date", index, iter->begin());
			result->set<LT::date>("Accrual End Date", index, iter->end());
            result->set<double>("Discount Factor", index, model.getDiscountFactor(ModuleDate::getYearsBetween(model.getValueDate(), iter->end())));
			fillSingleCashFlowPV(index, iter->begin(), iter->end(), model, *result);
			//result->set<double>("Rate", index, model.get
		}

		return result;
	}

}

#endif	//	__IDEA_FLEXYCF_BASELEG_H_INCLUDED
