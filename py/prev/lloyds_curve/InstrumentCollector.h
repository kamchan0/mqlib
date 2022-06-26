/*****************************************************************************

    InstrumentCollector

    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOLLECTOR_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOLLECTOR_H_INCLUDED

#include "LTQuantInitial.h"

//	FlexYCF
#include "KnotPointFunctor.h"
#include "LeastSquaresResiduals.h"
#include "Gradient.h"
#include "ResidualsUtils.H"


namespace FlexYCF
{
	class BaseModel;
	class MultiCurveModel;

	FWD_DECLARE_SMART_PTRS( CurveType )

	class InstrumentCollector : public IKnotPointFunctor
	{
	public:
		explicit InstrumentCollector(BaseModel* const model);
		
		inline void setInstrumentResidualRepresentationType(const LeastSquaresRepresentationType::Enum_t instrumentResidualsRepresentationType)
		{
			m_leastSquaresResiduals.setInstrumentResidualRepresentationType(instrumentResidualsRepresentationType);
		}

		// if necessary, could take an additional parameter: const bool knotPointVariableAdded
		virtual void operator() (KnotPoint & knotPoint) const;
		
		double evaluate(const size_t index) const;
		void update();
		// Obsolete:
		// void computeGradient(const size_t index, Gradient& gradient) const;
		void computeGradient(const size_t index, Gradient& gradient, const CurveTypeConstPtr& curveType) const;

		void clear();

		// Creates a least squares problem relative to the specified curve type
		LTQuant::LeastSquaresProblemPtr createLeastSquaresProblem(MultiCurveModel* const multiCurveModel,
														 const CurveTypeConstPtr& curveType);

	private:
		LeastSquaresResiduals	m_leastSquaresResiduals;
	};	
}

#endif //__LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOLLECTOR_H_INCLUDED