/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FIXEDLEG_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FIXEDLEG_H_INCLUDED
#pragma once
#include "FixedLegArguments.h"
#include "Gradient.h"
#include "BaseLeg.h"

namespace IDeA
{
    class AssetDomain;
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( FixedLeg )

    /// Represents a fixed leg as collection of
    /// (year fraction, discount factor).
    /// Note: the fixed leg schedule is constructed
    /// with the fixed leg arguments.
    class FixedLeg : public InstrumentComponent, 
                     public ICloneLookup,
					 public BaseLeg<FixedLegArguments>
    {
    private: 
        typedef std::pair<double, DiscountFactorPtr> CvgDfPair;  // (Coverage, Discount factor) pair
        typedef std::vector<CvgDfPair> CvgDfContainer;
        typedef CvgDfContainer::const_iterator const_iterator;

		template<class T>
		friend class FixedLegAdjusted;

    public:
        typedef FixedLegArguments Arguments;

		explicit FixedLeg(const FixedLegArguments& arguments):
			BaseLeg(arguments)
        {
         //   initialize(arguments);
        }
            
        static FixedLegPtr create(const FixedLegArguments& arguments)
        {
            return FixedLegPtr(new FixedLeg(arguments));
        }

        virtual double getValue(BaseModel const& baseModel);
        std::vector<std::pair<LT::date, double> > getAnnuity(BaseModel const& baseModel);

        virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd);
		virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);
		
		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
		{
			initializeCashFlowPricing();
			for(const_iterator iter(m_cashFlows.begin()); iter != m_cashFlows.end(); ++iter)
			{
				iter->second->accumulateGradientConstantDiscountFactor(baseModel, dfModel, multiplier * iter->first, gradientBegin, gradientEnd,spread);
			}
		}
		
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
		{
			initializeCashFlowPricing();
			for(const_iterator iter(m_cashFlows.begin()); iter != m_cashFlows.end(); ++iter)
			{
				iter->second->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier * iter->first, gradientBegin, gradientEnd,spread);
			}
		}
        
		virtual void update();

		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);

        virtual std::ostream& print(std::ostream& out) const;        

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
        
        LT::date getValueDate() const { return arguments().getValueDate(); }

		virtual std::vector<double>& getCvg()
		{
			return m_cvg;
		}
		
		virtual std::vector<Date>& getPaymentDates()
		{
			return m_paymentDates;
		}

		virtual ModuleDate::Schedule::ScheduleEvents& getSchedule()
		{
			return m_scheduleDates;
		}

        virtual void cleanupCashFlows();


    protected:
        FixedLeg(FixedLeg const& original, CloneLookup& lookup);

    private:
        // initialize the cash-flows
        //	void    initialize(const FixedLegArguments& arguments);
        FixedLeg(FixedLeg const&); // deliberately disabled as won't clone properly

		virtual std::string getLegTypeName() const;
		virtual void initializeCashFlows();
		virtual void doInitializeCashFlowPricing();
		virtual void fillSingleCashFlowPV(const size_t index,
										  const LT::date startDate,
										  const LT::date endDate,
										  const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable);

        CvgDfContainer  m_cashFlows;

		std::vector<double> m_cvg;

    };  //  FixedLeg

    DECLARE_SMART_PTRS( FixedLeg )

}   // FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FIXEDLEG_H_INCLUDED