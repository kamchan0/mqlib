/*****************************************************************************
    Class generating amortization schedule


    @Jarek Solowiej

    Copyright (C) Lloyds TSB Group plc 2013 All Rights Reserved
*****************************************************************************/

#include "stdafx.h"
#include "AmortizationSchedule.h"
#include "Matrix.h"
#include "Dictionaries.h"
#include "DataExtraction.h"

#include "Exception.h"

using namespace IDeA;

//const double AmortizationSchedule::defaultInitialNotional = 0.0;
const double AmortizationSchedule::defaultTargetNotional = 0.0;
const size_t AmortizationSchedule::defaultHolidayPeriods = 0;
const double AmortizationSchedule::defaultGrossGrowthRate = 0.0;
const double AmortizationSchedule::defaultSpecialEndAmount = 0.0;
const bool   AmortizationSchedule::defaultAdjustFirstPayment = false;
const bool   AmortizationSchedule::defaultEnforceConstantGrossPayments = false;
const double AmortizationSchedule::defaultConstantGrossPayments = 0.0;
const size_t AmortizationSchedule::defaultConstGrossPeriods = 0;
const int AmortizationSchedule::defaultRoundingDigits = 2;

AmortizationSchedule::AmortizationSchedule(double initialNotional, const std::vector<double>& couponRates,  const AmortizationType& atype, const LT::TablePtr&  amortizationDetails)
	: m_initialNotional(initialNotional), 
	m_couponRates(couponRates), 
	m_targetNotional(0.0), 
	m_holidayPeriods(0), 
	m_grossGrowthRate(0.0), 
	m_specialEndAmount(0.0), 
	m_constantGrossPayments(0.0), 
	m_constGrossPeriods(0),
	m_roundingDigits(2),
	m_roundingAdjFirstCapitalPayment(false),
	m_enforceConstantGrossPayments(false),
	m_notionals(couponRates.size(), initialNotional), 
	m_capitalPayments(couponRates.size(), 0.0),
	m_interestPayments(couponRates.size(), 0.0),
	m_amortizationType(atype)
{
	if(couponRates.empty())
	{
		LTQC_THROW(IDeA::MarketException, "AmortizationSchedule: no coupon payments");
	}

	//LT::Str mortizationTypeStr = IDeA::extract<LT::Str>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, AMORTIZATIONTYPE));
	//m_amortizationType = AmortizationType(mortizationTypeStr);

	IDeA::permissive_extract<double>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, TARGETNOTIONAL), m_targetNotional, 0.0);
	
	double holPeriods(0.0);
	IDeA::permissive_extract<double>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, HOLIDAYPERIODS), holPeriods, 0.0);
	m_holidayPeriods = static_cast<size_t>(holPeriods);
	if(m_holidayPeriods >= couponRates.size())
	{
		LTQC_THROW(IDeA::MarketException, "AmortizationSchedule: number of holiday periods " << m_holidayPeriods << " should be smaller than number of coupon payments " << couponRates.size());
	}


	if(m_constGrossPeriods >= couponRates.size())
	{
		LTQC_THROW(IDeA::MarketException, "AmortizationSchedule: number of constant gross payment periods " << m_constGrossPeriods << " should be smaller than number of coupon payments " << couponRates.size());
	}
	IDeA::permissive_extract<double>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, SPECIALENDAMOUNT), m_specialEndAmount, defaultSpecialEndAmount);
	IDeA::permissive_extract<double>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, GROWTHRATE), m_grossGrowthRate, 0.0);

 	//IDeA::permissive_extract<LT::date>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, MATURITYDATE), m_facilityMaturityDate, LT::date());
	

	LT::TablePtr capitalPayments;
	IDeA::permissive_extract<LT::TablePtr>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, CAPITALPAYMENTS ), capitalPayments, LT::TablePtr());
	if(capitalPayments)
	{
		 for(size_t i=0;i<std::min(m_capitalPayments.size(), capitalPayments->rowsGet());++i)
		 {
			m_capitalPayments[i] = capitalPayments->at(i,0);
		 }
	}

	double roundingDigits(0.0);
	IDeA::permissive_extract<double>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, ROUNDING), roundingDigits, 2.0);
	m_roundingDigits = static_cast<int>(roundingDigits);
	IDeA::permissive_extract<bool>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, ADJUSTFIRSTPAYMENT), m_roundingAdjFirstCapitalPayment, defaultAdjustFirstPayment);
	IDeA::permissive_extract<bool>(*amortizationDetails, IDeA_KEY(AMORTIZATIONDETAILS, ENFORCECONSTANTGROSSPAYMENTS), m_enforceConstantGrossPayments, defaultEnforceConstantGrossPayments);
	initilize();
}	

void AmortizationSchedule::initilize()
{
	if(m_couponRates.size() == 1)
	{
		generateBulletCapitalPayments();
	}
	else
	{
		switch(m_amortizationType)
		{
			case AmortizationType::BulletCapitalPayment:
				generateBulletCapitalPayments();
				break;
			case AmortizationType::StraightLineCapitalPayments:
				generateStrightLineCapitalPayments();
				break;
			case AmortizationType::FixedAnnuity:
			case AmortizationType::FloatAnnuity:
			case AmortizationType::ConstantGrossPayments:
				generateConstantGrossPayments();
				break;
			case AmortizationType::CustomGrossPayments:
				generateCustomGrossPayments();
				break;
			case AmortizationType::GrowingGrossPayments:
				generateGrowingGrossPayments();
				break;
			case AmortizationType::InterestOnlyGrossPayments:
				generateInterestOnlyGrossPayments();
				break;
			case AmortizationType::StepUpGrossPayments:
				generateStepUpGrossPayments();
				break;
			case AmortizationType::CustomCapitalPayments:
				generateCustomCapitalPayments();
				break;
			case AmortizationType::None:
				break;
			default:
				LTQC_THROW(IDeA::MarketException, "AmortizationSchedule: amortization type not implemented: " << m_amortizationType.asString().data());
		}
	}
	doRounding();
}


void AmortizationSchedule::doRounding()
{
	std::pair<LTQC::RoundingRules::Rule, int> rule = std::pair<LTQC::RoundingRules::Rule,int>(LTQC::RoundingRules::RoundToNearest, m_roundingDigits);
	m_notionals[0] = m_rounding.applyRule(m_notionals[0], rule);
	m_capitalPayments[0] = m_rounding.applyRule(m_capitalPayments[0], rule);
	m_interestPayments[0] = m_rounding.applyRule(m_couponRates[0] * m_notionals[0], rule);

	size_t k = m_couponRates.size();
	for(size_t j=1; j<k; ++j)
	{
		m_capitalPayments[j] = m_rounding.applyRule(m_capitalPayments[j], rule);
		m_notionals[j] = m_rounding.applyRule(m_notionals[j-1] - m_capitalPayments[j-1], rule);
		m_interestPayments[j] = m_rounding.applyRule(m_couponRates[j] * m_notionals[j], rule);
	}
	
	if(m_amortizationType == AmortizationType::CustomCapitalPayments)
	{
		return;
	}
	
	if(m_enforceConstantGrossPayments && (m_amortizationType == AmortizationType::FixedAnnuity || m_amortizationType == AmortizationType::ConstantGrossPayments) )
	{
		std::vector<double> repayments(m_couponRates.size() - m_holidayPeriods, 0.0);
		size_t m = m_couponRates.size()- m_holidayPeriods;
		for(size_t i=0; i< m - 1; ++i)
		{
			repayments[i] = m_interestPayments[i+m_holidayPeriods] + m_capitalPayments[i+m_holidayPeriods];
		}
		repayments[m-1] = m_interestPayments[m-1+m_holidayPeriods] + m_capitalPayments[m-1+m_holidayPeriods] - m_specialEndAmount;

		double average = std::accumulate(repayments.begin(),repayments.end(),0.0)/repayments.size();
		average = m_rounding.applyRule(average, rule);
		
		m_capitalPayments[m_holidayPeriods] = average - m_interestPayments[m_holidayPeriods];
		m = m_couponRates.size();
		for(size_t i=m_holidayPeriods + 1; i < m-1; ++i)
		{
			m_capitalPayments[i] = average - m_interestPayments[i];
			m_notionals[i] = m_notionals[i-1] - m_capitalPayments[i-1];
		}
		m_capitalPayments[m-1] = average - m_interestPayments[m-1] + m_specialEndAmount;
		m_notionals[m-1] = m_notionals[m-2] - m_capitalPayments[m-2];

		double residual = m_rounding.applyRule(m_notionals[0] - std::accumulate(m_capitalPayments.begin(),m_capitalPayments.end(),m_targetNotional), rule);

	
		if(m_roundingAdjFirstCapitalPayment && m_couponRates.size() > 1)
		{
			if(constantGrossPayments(average))
				return;
			else if(constantGrossPayments(average+0.01))
				return;
			else if(constantGrossPayments(average-0.01))
				return;
			else if(constantGrossPayments(average+0.02))
				return;
			else constantGrossPayments(average-0.02);	
		}
		else
		{
			m_capitalPayments[m-1] += residual; 
		}
	}
	else
	{
		double residual = m_rounding.applyRule(m_notionals[0] - std::accumulate(m_capitalPayments.begin(),m_capitalPayments.end(),m_targetNotional), rule);
		m_capitalPayments[k-1] += residual; 
	}
}

bool AmortizationSchedule::constantGrossPayments(double grossPayment)
{
	std::pair<LTQC::RoundingRules::Rule, int> rule = std::pair<LTQC::RoundingRules::Rule,int>(LTQC::RoundingRules::RoundToNearest, m_roundingDigits);
	size_t k = m_couponRates.size() - 1;
	m_notionals[k] = m_rounding.applyRule((grossPayment + m_targetNotional + m_specialEndAmount)/(1.0 + m_couponRates[k]), rule);
	m_capitalPayments[k] = m_notionals[k] - m_targetNotional;
	m_interestPayments[k] = m_rounding.applyRule(m_couponRates[k] * m_notionals[k], rule);
	for(size_t i = k - 1; i > m_holidayPeriods; --i)
	{
		m_notionals[i] = m_rounding.applyRule((grossPayment + m_notionals[i+1])/(1.0 + m_couponRates[i]), rule);
		m_capitalPayments[i] = m_notionals[i] - m_notionals[i+1];
		m_interestPayments[i] = m_rounding.applyRule(m_couponRates[i] * m_notionals[i], rule);
	}
	m_capitalPayments[m_holidayPeriods] = m_notionals[m_holidayPeriods] - m_notionals[m_holidayPeriods + 1];
	m_interestPayments[m_holidayPeriods] = m_rounding.applyRule(m_couponRates[m_holidayPeriods] * m_notionals[m_holidayPeriods], rule);

	for(size_t i = k-1; i > m_holidayPeriods + 1; --i)
	{
		if(std::fabs(grossPayment - m_interestPayments[i] - m_capitalPayments[i]) > 0.001)
		{ 
			return false;
		}
	}
	return true;
}

void AmortizationSchedule::generateBulletCapitalPayments()
{
	size_t k             = m_couponRates.size() - 1;
	m_capitalPayments[k] = m_initialNotional - m_targetNotional + m_specialEndAmount;
}

void AmortizationSchedule::generateStrightLineCapitalPayments()
{
	size_t k             = m_couponRates.size();
	m_capitalPayments[m_holidayPeriods] = (m_initialNotional - m_targetNotional - m_specialEndAmount)/(k - m_holidayPeriods);
		
	for(size_t j=m_holidayPeriods+1; j<k-1; ++j)
	{
		m_notionals[j] = m_notionals[j-1] - m_capitalPayments[j-1];
		m_capitalPayments[j] = m_capitalPayments[m_holidayPeriods];
	}
	m_notionals[k-1] = m_notionals[k-2] - m_capitalPayments[k-2];
	m_capitalPayments[k-1] = m_capitalPayments[m_holidayPeriods] + m_specialEndAmount;
}

void AmortizationSchedule::generateCustomCapitalPayments()
{
	 size_t k = m_couponRates.size();
	 for(size_t j = 1; j < k; ++j)
	 {
		m_notionals[j] = m_notionals[j-1] - m_capitalPayments[j-1];
	 }
}

void AmortizationSchedule::generateConstantGrossPayments()
{
	size_t k = m_couponRates.size() + 1;
	 LTQC::Matrix w(k,k, 0.0);

	 for(size_t row=0; row < k - 1; ++row)
	 {
		 w(row,row)     =  1.0;
		 if(row >= m_holidayPeriods)
		 {
			 w(row,k-1)   = -1.0;
			 for(size_t j=0; j<row; ++j)
			 {
				w(row,j)   =  - m_couponRates[row];
			 }
		 }
	 }
	 for(size_t j=m_holidayPeriods; j < k - 1; ++j)
	 {
		w(k-1,j)   =  1.0;
	 }

	 LTQC::VectorDouble rhs(k, 0.0), result;
	 for(size_t j=m_holidayPeriods; j < k - 2; ++j)
	 {
		rhs[j]   =  - m_couponRates[j] * m_initialNotional;
	 }
	 rhs[k-2] =  - m_couponRates[k-2] * m_initialNotional + m_specialEndAmount;
	 rhs[k-1] = m_initialNotional - m_targetNotional;

	 w.inverse();
	 LTQC::Matrix::dotRight(w, rhs, result);
	 
	 m_capitalPayments[m_holidayPeriods] = result[m_holidayPeriods];
	 for(size_t j = m_holidayPeriods + 1; j < k-1; ++j)
	 {
		m_notionals[j] = m_notionals[j-1] - m_capitalPayments[j-1];
		m_capitalPayments[j] = result[j];
	 }
}

void AmortizationSchedule::generateInterestOnlyGrossPayments()
{
	size_t k             = m_couponRates.size() - 1;
	m_capitalPayments[k] = m_specialEndAmount;
}

void AmortizationSchedule::generateCustomGrossPayments()
{
	size_t k = m_couponRates.size();
	 LTQC::Matrix w(k,k, 0.0);

	 for(size_t row=0; row < k; ++row)
	 {
		w(row,row)     =  1.0;
		for(size_t j=0; j<row; ++j)
		{
			w(row,j)   =  - m_couponRates[row];
		}
	 }

	 LTQC::VectorDouble rhs(k, 0.0), result;
	 for(size_t j=0; j < k; ++j)
	 {
		rhs[j]   =  - m_couponRates[j] * m_initialNotional + m_grossPayments[j];
	 }
	

	 w.inverse();
	 LTQC::Matrix::dotRight(w, rhs, result);
	 
	 m_capitalPayments[0] = result[0];
	 for(size_t j = 1; j < k; ++j)
	 {
		m_notionals[j] = m_notionals[j-1] - m_capitalPayments[j-1];
		m_capitalPayments[j] = result[j];
	 }
}

void AmortizationSchedule::generateStepUpGrossPayments()
{
	size_t k = m_couponRates.size() + 1;
	 LTQC::Matrix w(k,k, 0.0);

	 for(size_t row=0; row < m_constGrossPeriods; ++row)
	 {
		w(row,row)     =  1.0;
		for(size_t j=0; j<row; ++j)
		{
			w(row,j)   =  - m_couponRates[row];
		}
	 }
	 
	 for(size_t row = m_constGrossPeriods; row < k - 1; ++row)
	 {
		w(row,row)     =  1.0;
		w(row,k-1)     = -1.0;
		for(size_t j=0; j<row; ++j)
		{
			w(row,j)   =  - m_couponRates[row];
		}
	 }
	 
	 for(size_t j=0; j < k - 1; ++j)
	 {
		w(k-1,j)   =  1.0;
	 }

	 LTQC::VectorDouble rhs(k, 0.0), result;
	 for(size_t j = 0; j < m_constGrossPeriods; ++j)
	 {
		rhs[j]   =  - m_couponRates[j] * m_initialNotional + m_constantGrossPayments;
	 }
	 for(size_t j = m_constGrossPeriods; j < k - 2; ++j)
	 {
		rhs[j]   =  - m_couponRates[j] * m_initialNotional;
	 }
	 rhs[k-2] =  - m_couponRates[k-2] * m_initialNotional + m_specialEndAmount;
	 rhs[k-1] = m_initialNotional - m_targetNotional;

	 w.inverse();
	 LTQC::Matrix::dotRight(w, rhs, result);
	 
	 m_capitalPayments[0] = result[0];
	 for(size_t j = 1; j < k; ++j)
	 {
		m_notionals[j] = m_notionals[j-1] - m_capitalPayments[j-1];
		m_capitalPayments[j] = result[j];
	 }
}

void AmortizationSchedule::generateGrowingGrossPayments()
{
	size_t k = m_couponRates.size() + 1;
	 LTQC::Matrix w(k,k, 0.0);

	 for(size_t row=0; row < m_constGrossPeriods; ++row)
	 {
		w(row,row)     =  1.0;
		for(size_t j=0; j<row; ++j)
		{
			w(row,j)   =  - m_couponRates[row];
		}
	 }
	 
	 for(size_t row = m_constGrossPeriods; row < k - 1; ++row)
	 {
		w(row,row)     =  1.0;
		w(row,k-1)     = - ::pow(1.0 + m_grossGrowthRate, double(row - m_constGrossPeriods));
		for(size_t j=0; j<row; ++j)
		{
			w(row,j)   =  - m_couponRates[row];
		}
	 }
	 
	 for(size_t j=0; j < k - 1; ++j)
	 {
		w(k-1,j)   =  1.0;
	 }

	 LTQC::VectorDouble rhs(k, 0.0), result;
	 for(size_t j = 0; j < m_constGrossPeriods; ++j)
	 {
		rhs[j]   =  - m_couponRates[j] * m_initialNotional + m_constantGrossPayments;
	 }
	 for(size_t j = m_constGrossPeriods; j < k - 2; ++j)
	 {
		rhs[j]   =  - m_couponRates[j] * m_initialNotional;
	 }
	 rhs[k-2] =  - m_couponRates[k-2] * m_initialNotional + m_specialEndAmount;
	 rhs[k-1] = m_initialNotional - m_targetNotional;

	 w.inverse();
	 LTQC::Matrix::dotRight(w, rhs, result);
	 
	 m_capitalPayments[0] = result[0];
	 for(size_t j = 1; j < k; ++j)
	 {
		m_notionals[j] = m_notionals[j-1] - m_capitalPayments[j-1];
		m_capitalPayments[j] = result[j];
	 }
}