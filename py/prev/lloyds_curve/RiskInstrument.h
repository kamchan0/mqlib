/*****************************************************************************
	RiskInstrument

	Base class that represents an instrument against which risk 
	can be calculated and encapsulate data common to calibration and
	structure instruments.

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __RiskInstrument_H__
#define __RiskInstrument_H__


//	LTQC
#include "Macros.h"

//	LTQuantLib
#include "Maths/Problem.h"

//	FlexYCF
#include "SolverVariable.h"


namespace FlexYCF
{

	class RiskInstrument
	{
	public:
		RiskInstrument(const double rate,
					   const LT::Str& name,
					   const LT::Str& description):
			m_rate(rate),
			m_name(name),
			m_description(description)
		{
		}
		
		//	Returns the market rate (or spread) of the risk instrument
		inline double getRate() const
		{
			return m_rate;
		}

		/// Sets the rate of the instrument
		inline void setRate(const double rate)      
		{ 
			m_rate = rate;
		}

		//	Returns the name of the instrument
		inline const LT::Str& getName() const
		{
			return m_name;
		}

		//	Returns the description of the risk instrument
		//	Note: most of the time, this is its maturity
		inline const LT::Str& getDescription() const
		{
			return m_description;
		}
	
		// Add the instrument rate as a variable to the specified problem
		inline void addRateToProblem(LTQuant::Problem& problem)
		{
			problem.addVariable(LTQuant::VariablePtr(new SolverVariable(m_rate)));
		}

		
		//	Shifts the rate of the calibration instrument by the specified shift
		inline void shiftRate(const double shift)
		{
			setRate(getRate() + shift);
		}

	protected:
		RiskInstrument(){ }

	private:
		double	m_rate;
		LT::Str	m_name;
		LT::Str	m_description;
	};

	LTQC_DECLARE_SMART_PTRS( RiskInstrument )
}



#endif	//	__RiskInstrument_H__