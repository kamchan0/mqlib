/*****************************************************************************

	BreakoutInitialization

	Breaks out the problem to solve into a set of consistent, easier to
	solve problems.
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BREAKOUTINITIALIZATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BREAKOUTINITIALIZATION_H_INCLUDED

#include "LTQuantInitial.h"

//	FlexYCF
#include "BaseInitialization.h"
#include "CurveType.h"
#include "ResidualsUtils.h"


namespace LTQuant
{
	FWD_DECLARE_SMART_PTRS( LeastSquaresSolver )	
	FWD_DECLARE_SMART_PTRS( LeastSquaresProblem )
}

namespace FlexYCF
{
	class InstrumentCollector;
	class MultiCurveModel;

	FWD_DECLARE_SMART_PTRS( CurveType)


	/// Separate initialization for base, Tenor and funding curve.
	class BreakoutInitialization : public BaseInitialization
	{
	private:
		typedef std::map<CurveTypeConstPtr, double> InitialSpotRates;
		typedef std::vector<CurveTypeConstPtr>		CurveOrder;

	public:
		explicit BreakoutInitialization(const LTQuant::LeastSquaresSolverPtr& leastSquaresSolver,
									    const InitialSpotRates& initialSpotRates,
									    const double defaultSpotRate,
									    const CurveOrder& curveOrder,
										const LeastSquaresRepresentationType::Enum_t lsrType);

		virtual ~BreakoutInitialization() { }
		
		static std::string getName();
		static BaseInitializationPtr create(const LTQuant::GenericDataPtr& initializationTable);
	
	private:
		virtual void doInitialize(BaseModel* const model) const;

		void solveCurve(InstrumentCollector& instrumentCollector,
						MultiCurveModel* const model,
						const CurveTypeConstPtr& curveType) const;
		
		LTQuant::LeastSquaresSolverPtr		    m_leastSquaresSolver;
		InitialSpotRates						m_initialSpotRates;
		double									m_defaultSpotRate;
		CurveOrder								m_curveOrder;
		LeastSquaresRepresentationType::Enum_t	m_lsrType;
	};
}
#endif // __LIBRARY_PRICERS_FLEXYCF_BreakoutInitialization_H_INCLUDED