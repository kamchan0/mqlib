/*****************************************************************************

    ForwardRate

	Represents a single forward rate as an instrument component
	in a pricing formula.

	
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FORWARDRATE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FORWARDRATE_H_INCLUDED
#pragma once

//	FlexYCF
#include "ForwardRateArguments.h"
#include "GenericInstrumentComponent.h"
#include "TenorDiscountFactor.h"
#include "Gradient.h"
#include "IHasRepFlows.h"
#include "IndexRepFlow.h"
#include "StubUtils.h"
#include "TenorUtils.h"

//	LTQuantLib
#include "ModuleDate/InternalInterface/CalendarDuration.h"

//	LTQC
#include "Tenor.h"

//	IDeA
#include "IDeA\src\market\MarketConvention.h"

// FlexYCF
#include "ICloneLookup.h"

namespace IDeA
{
    class AssetDomain;
}

namespace FlexYCF
{
    /// Represens a forward rate in the pricing formula
    /// of a calibration instrument.
    class ForwardRate : public InstrumentComponent,
                        public ICloneLookup,
                        public IHasRepFlows<IDeA::Index>
    {
    private:
        DECLARE_SMART_PTRS( ForwardRate )

    public:
        typedef ForwardRateArguments Arguments;

        explicit ForwardRate(const ForwardRateArguments& arguments):
            m_arguments(arguments),
            m_startDateTenorDiscountFactor(arguments.getStartDateTenorDiscountFactor()),
            m_endDateTenorDiscountFactor(arguments.getEndDateTenorDiscountFactor()),
            m_coverage(arguments.getCoverage()),
            m_coverageInverse(arguments.getCoverageInverse()),
            m_tenor(arguments.getTenor()),
            m_timeToExpiry(arguments.getTimeToExpiry()),
            m_timeToMaturity(arguments.getTimeToMaturity()),
			m_ltqc_tenor(tenorEquivalency(arguments.getTenorDescription()))
        {
        }

        static ForwardRatePtr create(const ForwardRateArguments& arguments)
        {
            return ForwardRatePtr(new ForwardRate(arguments));
        }

		//	Calculates fixing, index start and index end dates from accrual start date and deposit rate market conventions
		//	(in particular, endDate is (roughly) equal startDate + Tenor
		static void calcIndexDates(LT::date& fixingDate, LT::date& startDate, LT::date& endDate, const LT::date& accStartDate, const IDeA::DepositRateMktConvention& rateDetails);
		
		//	General function to calculate the index dates, and in particular the end date 
		//	as specified by the end date calculate type
		static void calculateIndexDates(LT::date& fixingDate,
										LT::date& startDate,
										LT::date& endDate,
										const LT::date accStartDate,
										const LT::date accEndDate,
										const IDeA::DepositRateMktConvention& rateDetails,
                                        const ModuleDate::DayCounterConstPtr& rateBasis,
										const EndDateCalculationType endDateCalculationType = EndDateCalculationType::NotAdjusted);
	
        virtual double getValue(BaseModel const& baseModel);
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
										bool spread);
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);
		
        virtual void update();

        // provided for compatibility with DiscountedForwardRate
        inline const Arguments& getArguments() const
        {
            return m_arguments;
        }

        // provided for compatibility with DiscountedForwardRate
        inline bool operator==(const ForwardRate& forwardRate) const
        {
            return m_arguments == forwardRate.m_arguments;
        }

        inline double getCoverage() const
        {
            return m_coverage;
        }

        inline double getTenor() const
        {
            return m_tenor;
        }

        inline double getTimeToExpiry() const
        {
            return m_timeToExpiry;
        }

        inline double getTimeToMaturity() const
        {
            return m_timeToMaturity;
        }

        inline double getLastRelevantTime() const
        {
            return m_arguments.getLastRelevantTime();
        }

        LT::date getFixingDate() const;
        LT::date getStartDate() const;
        LT::date getEndDate() const;

		//	IHasRepFlows<IDeA::Index> interface
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
								  const double multiplier,
								  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);

        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        ForwardRate(ForwardRate const& original, CloneLookup& lookup);

    private:
        ForwardRate(ForwardRate const&); // deliberately disabled as won't clone properly

        ForwardRateArguments	m_arguments;   // repeated here for now for compatibility with DiscountedForwardRate
        TenorDiscountFactorPtr	m_startDateTenorDiscountFactor;
        TenorDiscountFactorPtr	m_endDateTenorDiscountFactor;
        double					m_coverage;
        double					m_coverageInverse;
        double					m_tenor;
        double					m_timeToExpiry;
        double					m_timeToMaturity;
		LTQC::Tenor				m_ltqc_tenor;
    };  //  ForwardRate

    DECLARE_SMART_PTRS( ForwardRate )
	size_t hash_value(const ForwardRate& forwardRate);

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FORWARDRATE_H_INCLUDED