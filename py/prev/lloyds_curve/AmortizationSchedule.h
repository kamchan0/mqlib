#ifndef __AmortizationSchedule_H__
#define __AmortizationSchedule_H__

/*****************************************************************************
    Class generating amortization schedule


    @Jarek Solowiej

    Copyright (C) Lloyds TSB Group plc 2013 All Rights Reserved
*****************************************************************************/

#include "Library/PublicInc/Date.h"
#include "lt/table.h"
#include "Rounding.h"
#include "src/Enums/AmortizationType.h"

namespace IDeA
{
	class AmortizationSchedule {
	public:
		AmortizationSchedule(double initialNotional, const std::vector<double>& couponRates,  const AmortizationType& atype, const LT::TablePtr&  amortizationDetails);
	
		// returns i-th notional
		double notional(size_t i) const { return m_notionals[i]; }
		
		// returns i-th capital payment
		double capitalPayment(size_t i) const { return m_capitalPayments[i]; }
		double couponPayment(size_t i) const { return m_interestPayments[i]; }

		size_t size() const { return m_notionals.size(); };

		static const double defaultTargetNotional;
		static const size_t defaultHolidayPeriods;
		static const double defaultGrossGrowthRate;
		static const double defaultSpecialEndAmount;
		static const bool   defaultAdjustFirstPayment;
		static const bool   defaultEnforceConstantGrossPayments;
		static const double defaultConstantGrossPayments;
		static const size_t defaultConstGrossPeriods;
		static const int defaultRoundingDigits;


	private:
		
		void initilize();
		void doRounding();
		bool constantGrossPayments(double grossPayment);

		void generateBulletCapitalPayments();
		void generateStrightLineCapitalPayments();
		void generateCustomCapitalPayments();
		void generateInterestOnlyGrossPayments();
		void generateConstantGrossPayments();
		void generateCustomGrossPayments();
		void generateStepUpGrossPayments();
		void generateGrowingGrossPayments();

		double			    m_initialNotional;
		std::vector<double> m_couponRates;
		const AmortizationType&   m_amortizationType;
		double			    m_targetNotional;
		size_t              m_holidayPeriods;
		double				m_grossGrowthRate;
		LT::date		    m_facilityMaturityDate;
		double			    m_specialEndAmount;
		double			    m_constantGrossPayments;
		size_t              m_constGrossPeriods;
		LTQC::RoundingRules	m_rounding;
		int 				m_roundingDigits;
		bool                m_roundingAdjFirstCapitalPayment;
		bool				m_enforceConstantGrossPayments;

		std::vector<double> m_grossPayments;
		std::vector<double> m_capitalPayments;
		std::vector<double> m_notionals;
		std::vector<double> m_interestPayments;
	};
}

#endif