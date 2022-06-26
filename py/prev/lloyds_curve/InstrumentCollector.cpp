/*****************************************************************************
    
	InstrumentCollector

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InstrumentCollector.h"
#include "BaseModel.h"
#include "MultiCurveModel.h"
#include "Maths/LeastSquaresProblem.h"

using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{
	InstrumentCollector::InstrumentCollector(BaseModel* const model):
		m_leastSquaresResiduals(model)
	{
	}

	void InstrumentCollector::operator() (KnotPoint & knotPoint) const
	{
		if(knotPoint.getRelatedInstrument())
		{
			const_cast<InstrumentCollector*>(this)->m_leastSquaresResiduals.addInstrumentResidual(knotPoint.getRelatedInstrument());
			LT_LOG << (*(knotPoint.getRelatedInstrument())) << endl;
		}
	}

	double InstrumentCollector::evaluate(const size_t index) const
	{
		return m_leastSquaresResiduals.evaluate(index);
	}

	void InstrumentCollector::update()
	{
		m_leastSquaresResiduals.update();
	}
	/*
	void InstrumentCollector::computeGradient(const size_t index, std::vector<double>& gradient) const
	{
		m_leastSquaresResiduals.computeGradient(index, gradient);
	}
	*/
	void InstrumentCollector::computeGradient(const size_t index, Gradient& gradient, const CurveTypeConstPtr& curveType) const
	{
		m_leastSquaresResiduals.computeGradient(index, gradient, curveType);
	}

	void InstrumentCollector::clear()
	{
		m_leastSquaresResiduals.clear();
	}
	
	LeastSquaresProblemPtr InstrumentCollector::createLeastSquaresProblem(MultiCurveModel* const multiCurveModel,
																		  const CurveTypeConstPtr& curveType)
	{
		return LeastSquaresProblemPtr(
			new LeastSquaresProblem(multiCurveModel->getNumberOfUnknowns(curveType),
									[this] (size_t index) {return evaluate(index);},
									[this] () {update();},
									[this, curveType] (size_t index, FlexYCF::Gradient& gradient)
										{return computeGradient(index, gradient, curveType);}));
	}
}