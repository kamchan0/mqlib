/*****************************************************************************
    
	InstrumentResidual


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "InstrumentResidual.h"
#include "CalibrationInstrument.h"
#include "BaseModel.h"
#include "FlexYCFCloneLookup.h"

//	LTQuantCore
#include "QCException.h"


using namespace std;

using namespace LTQC;

namespace FlexYCF
{
    
    InstrumentResidual::InstrumentResidual(const BaseModelPtr baseModel,
                                           const CalibrationInstrumentPtr instrument, 
                                           const double weight):
        WeightedResidual(weight),
        m_baseModel(baseModel),
        m_instrument(instrument)
    {
    }

    void InstrumentResidual::update()
    {
        m_instrument->update();
    }

    double InstrumentResidual::getValueImpl() const 
    {
		if(LeastSquaresRepresentationType::PV == m_representationType)
		{
			return m_instrument->getResidual(m_baseModel);
		}
		else if(LeastSquaresRepresentationType::Rate == m_representationType)
		{
			return m_instrument->getResidual(m_baseModel) / m_instrument->calculateRateDerivative(m_baseModel);
		}
		else
		{
			LTQC_THROW( LTQC::ModelQCException, "Invalid representation type" );
		}
    }

    void InstrumentResidual::computeGradient(Gradient& gradient) const
    {
		if(LeastSquaresRepresentationType::PV == m_representationType)
		{
			m_instrument->accumulateGradient(*m_baseModel, getWeight(), gradient.begin(), gradient.end());
		}
		else if(LeastSquaresRepresentationType::Rate == m_representationType)
		{
			// instrument residual gradient on rate = Dr / B - (r / B^2) DB
			//	where: 
			//	D: gradient operator, relative to the unknowns
			//	r: instrument residual on PV
			//	B: rate derivative = 10^4 * BPV
			const double inverseRateDerivative(1.0 / m_instrument->calculateRateDerivative(m_baseModel));
			m_instrument->accumulateGradient(*m_baseModel, inverseRateDerivative * getWeight(), gradient.begin(), gradient.end());

			m_instrument->accumulateRateDerivativeGradient(*m_baseModel, 
				- m_instrument->getResidual(m_baseModel) * inverseRateDerivative * inverseRateDerivative * getWeight(),
				gradient.begin(), gradient.end());
		}
		else
		{
			LTQC_THROW( LTQC::ModelQCException, "Invalid representation type" );
		}
    }

	void InstrumentResidual::computeGradient(Gradient& gradient, 
											 const CurveTypeConstPtr& curveType) const
	{
		if(LeastSquaresRepresentationType::PV == m_representationType)
		{
			m_instrument->accumulateGradient(*m_baseModel, getWeight(), gradient.begin(), gradient.end(), curveType);
		}
		else if(LeastSquaresRepresentationType::Rate == m_representationType)
		{
			const double inverseRateDerivative(1.0 / m_instrument->calculateRateDerivative(m_baseModel));
			m_instrument->accumulateGradient(*m_baseModel, inverseRateDerivative * getWeight(), gradient.begin(), gradient.end(), curveType);

			m_instrument->accumulateRateDerivativeGradient(*m_baseModel, 
				- m_instrument->getResidual(m_baseModel) * inverseRateDerivative * inverseRateDerivative * getWeight(),
				gradient.begin(), gradient.end(), curveType);
		}
		else
		{
			LTQC_THROW( LTQC::ModelQCException, "Invalid representation type" );
		}
	}

    std::ostream& InstrumentResidual::print(std::ostream& out) const
    {
        double residual(getValueImpl());
        out << m_instrument->getType() << m_instrument->getDescription().string() << ":\t" << residual;
        return out;
    }

    /**
        @brief Clone this instrument residual.

        Ensure that existing clones of the BaseModel and CalibrationInstrument are reused.

        @param lookup The lookup table to preserve directed graph relationships.
        @return       The clone of this instance.
    */
    ICloneLookupPtr InstrumentResidual::cloneWithLookup(CloneLookup& lookup) const
    {
        return InstrumentResidualPtr(new InstrumentResidual(*this, lookup));
    }

    /**
        @brief Copy constructor.

        A lookup table is used to ensure directed graph relationships are preserved.

        @param instance The instance to copy.
        @param lookup   The lookup of previous clones.
    */
    InstrumentResidual::InstrumentResidual(InstrumentResidual const& instance, CloneLookup& lookup) :
        WeightedResidual(instance), 
        m_baseModel(lookup.get(instance.m_baseModel)), 
        m_instrument(lookup.get(instance.m_instrument))
    {
    }

}   //  FlexYCF