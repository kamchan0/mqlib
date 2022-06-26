/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "ForwardRate.h"
#include "ForwardRateArguments.h"
#include "BaseModel.h"
#include "TenorDiscountFactor.h"
#include "RepFlowsData.h"
#include "FlexYCFCloneLookup.h"

//	LTQuantLib
#include "DateUtils.h"
#include "ModuleDate/InternalInterface/CalendarDuration.h"

// dates
#include "QDDate/Date.h"
#include "QDDate/DatePeriod.h"
#include "QDDate/DateDuration.h"

// IDeA
#include "AssetDomain.h"
#include "MarketConvention.h"
#include "TradeConventionCalcH.h"

// LTQC
#include "Tenor.h"
#include "RollConv.h"


using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    // Let K be a knot-point y-value. As  F(t,T)[K] = [P(t)[K] / P(T)[K] - 1] / cvg    
    double ForwardRate::getValue(BaseModel const& baseModel)
    {
        return m_coverageInverse * (m_startDateTenorDiscountFactor->getValue(baseModel) / m_endDateTenorDiscountFactor->getValue(baseModel) - 1.0);
    }

    // Let K be a knot-point y-value. As  F(t,T)[K] = [P(t)[K] / P(T)[K] - 1] / cvg    
    //  we have: dF / dK = { P(T) * dP(t)/dK  -  P(t) * dP(T)/dK} / (cvg * P(T)^2) 
    void ForwardRate::accumulateGradient(BaseModel const& baseModel, 
                                         double multiplier,
                                         GradientIterator gradientBegin,
                                         GradientIterator gradientEnd)
    {
        // left side inside the brackets of the above formula
        double payDcf(m_endDateTenorDiscountFactor->getValue(baseModel));
        double setDcf(m_startDateTenorDiscountFactor->getValue(baseModel));
        double denom(m_coverage * payDcf * payDcf);

        m_startDateTenorDiscountFactor->accumulateGradient(baseModel, multiplier * payDcf / denom, gradientBegin, gradientEnd);
         
        // right side of inside the brackets of the above formula
        m_endDateTenorDiscountFactor->accumulateGradient(baseModel, -multiplier * setDcf / denom, gradientBegin, gradientEnd);
    }

	void ForwardRate::accumulateGradient(BaseModel const& baseModel, 
                                         double multiplier, 
                                         GradientIterator gradientBegin, 
                                         GradientIterator gradientEnd,
										 const CurveTypeConstPtr& curveType)
	{
		// left side inside the brackets of the above formula
        double payDcf(m_endDateTenorDiscountFactor->getValue(baseModel));
        double setDcf(m_startDateTenorDiscountFactor->getValue(baseModel));
        double denom(m_coverage * payDcf * payDcf);

        m_startDateTenorDiscountFactor->accumulateGradient(baseModel, multiplier * payDcf / denom, gradientBegin, gradientEnd, curveType);
         
        // right side of inside the brackets of the above formula
        m_endDateTenorDiscountFactor->accumulateGradient(baseModel, -multiplier * setDcf / denom, gradientBegin, gradientEnd, curveType);
	}
	
	void ForwardRate::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
	
	void ForwardRate::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		double endDf   = m_endDateTenorDiscountFactor->getValue(dfModel);
		double startDf = m_startDateTenorDiscountFactor->getValue(dfModel);
			
		m_startDateTenorDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, multiplier * m_coverageInverse / endDf, gradientBegin, gradientEnd, spread);
		m_endDateTenorDiscountFactor->accumulateGradientConstantTenorDiscountFactor(baseModel, dfModel, - multiplier * m_coverageInverse * startDf / ( endDf * endDf ), gradientBegin, gradientEnd, spread);
	}

    void ForwardRate::update()
    {
        m_startDateTenorDiscountFactor->update();
        m_endDateTenorDiscountFactor->update();
    }
 
    LT::date ForwardRate::getFixingDate() const
    {
        return m_arguments.getFixingDate();
    }
	
	LT::date ForwardRate::getStartDate() const
    {
        return m_arguments.getStartDate();
    }

    LT::date ForwardRate::getEndDate() const
    {
        return m_arguments.getEndDate();
    }

	void ForwardRate::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                   const BaseModel& model,
								   const double multiplier,
								   IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		//	Add the derivative relative the index discount factors of the forward rate:
        indexRepFlows.addRepFlow(IDeA::Index::Key(assetDomain, m_arguments.getStartDate(), m_startDateTenorDiscountFactor->getArguments().getTenor()), 
								 multiplier * m_coverageInverse / m_endDateTenorDiscountFactor->getValue(model));
        indexRepFlows.addRepFlow(IDeA::Index::Key(assetDomain, m_arguments.getEndDate(),  m_endDateTenorDiscountFactor->getArguments().getTenor()), 
								 -multiplier * m_coverageInverse * m_startDateTenorDiscountFactor->getValue(model) / pow(m_endDateTenorDiscountFactor->getValue(model), 2.0));
	}

    ostream& ForwardRate::print(ostream& out) const
    {
        out << m_arguments;
        return out;
    }

	
	void ForwardRate::calcIndexDates(LT::date& fixingDate, LT::date& startDate, LT::date& endDate, const LT::date& accStartDate, const IDeA::DepositRateMktConvention& rateDetails)
	{
		Date sDate, eDate;
		Date fDate = IDeA::IRDepositRateCalcH::calculateFixingDate(Date(accStartDate), rateDetails);
		IDeA::IRDepositRateCalcH::calculateValueDates(sDate, eDate, fDate, rateDetails.m_rateTenor, rateDetails.m_spotDays, rateDetails.m_fixingCalendar, rateDetails.m_accrualValueCalendar, 
			rateDetails.m_rollConvention, rateDetails.m_rollRuleConvention);
		fixingDate = fDate.getAsLTdate();
		startDate = sDate.getAsLTdate();
		endDate = eDate.getAsLTdate();
	}

	void ForwardRate::calculateIndexDates(LT::date& fixingDate,
										  LT::date& startDate,
										  LT::date& endDate,
										  const LT::date accStartDate,
										  const LT::date accEndDate,
										  const IDeA::DepositRateMktConvention& rateDetails,
										  const ModuleDate::DayCounterConstPtr& rateBasis,
										  const EndDateCalculationType endDateCalculationType)
	{
		//	Calculate fixing date from accrual start date
		const Date fDate(IDeA::IRDepositRateCalcH::calculateFixingDate(Date(accStartDate), rateDetails));

		Date sDate, eDate;

		const double accrualPeriod(rateBasis->getDaysOverBasis(accStartDate, accEndDate));

		switch(endDateCalculationType)
		{
		case EndDateCalculationType::NotAdjusted:
			IDeA::IRDepositRateCalcH::calculateValueDates(sDate, eDate, fDate, 
														  rateDetails.m_rateTenor, 
														  rateDetails.m_spotDays,
														  rateDetails.m_fixingCalendar,
														  rateDetails.m_accrualValueCalendar, 
														  rateDetails.m_rollConvention,
														  rateDetails.m_rollRuleConvention);
			break;
		case EndDateCalculationType::FromAccrualEndDate:
			//	Use the same function to calculate the start date
			IDeA::IRDepositRateCalcH::calculateValueDates(sDate, eDate, fDate, 
														  rateDetails.m_rateTenor, 
														  rateDetails.m_spotDays,
														  rateDetails.m_fixingCalendar,
														  rateDetails.m_accrualValueCalendar, 
														  rateDetails.m_rollConvention,
														  rateDetails.m_rollRuleConvention);
			//	Set the end date to the accrual end date
			eDate = accEndDate;
			break;
		case EndDateCalculationType::FromStartDatePlusTenor:
			//	If the closest tenor to the accrual period is less than the tenor of the forward rate,
			//	then use it instead to calculate the end date
			IDeA::IRDepositRateCalcH::calculateValueDates(sDate, eDate, fDate, 
				CurveType::DereferenceLess()(CurveType::getFromYearFraction(accrualPeriod),
											 CurveType::getFromYearFraction(rateDetails.m_rateTenor.asYearFraction()))?
														  LTQC::Tenor(CurveType::getFromYearFraction(accrualPeriod)->getDescription()):
														  rateDetails.m_rateTenor,
														  rateDetails.m_spotDays,
														  rateDetails.m_fixingCalendar,
														  rateDetails.m_accrualValueCalendar, 
														  rateDetails.m_rollConvention,
														  rateDetails.m_rollRuleConvention);
			break;
		default:
			IDeA::IRDepositRateCalcH::calculateValueDates(sDate, eDate, fDate, 
														  rateDetails.m_rateTenor, 
														  rateDetails.m_spotDays,
														  rateDetails.m_fixingCalendar,
														  rateDetails.m_accrualValueCalendar, 
														  rateDetails.m_rollConvention,
														  rateDetails.m_rollRuleConvention);
		}

		//	Convert to boost dates
		fixingDate	= fDate.getAsLTdate();
		startDate	= sDate.getAsLTdate();
		endDate		= eDate.getAsLTdate();		
	}

    /**
        @brief Clone this instance.

        Use a lookup to preserve directed graph relationships.

        @param lookup A lookup of previously created clones.
        @return       Returns a clone.
    */
    ICloneLookupPtr ForwardRate::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new ForwardRate(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Use a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    ForwardRate::ForwardRate(ForwardRate const& original, CloneLookup& lookup) : 
        m_arguments(original.m_arguments, lookup),
        m_startDateTenorDiscountFactor(lookup.get(original.m_startDateTenorDiscountFactor)),
        m_endDateTenorDiscountFactor(lookup.get(original.m_endDateTenorDiscountFactor)),
        m_coverage(original.m_coverage),
        m_coverageInverse(original.m_coverageInverse),
        m_tenor(original.m_tenor),
        m_timeToExpiry(original.m_timeToExpiry),
        m_timeToMaturity(original.m_timeToMaturity),
		m_ltqc_tenor(original.m_ltqc_tenor)
    {
    }

}   // FlexYCF