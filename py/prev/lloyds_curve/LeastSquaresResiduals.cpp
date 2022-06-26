/*****************************************************************************
    
	LeastSquaresResiduals

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "LeastSquaresResiduals.h"
#include "BaseModel.h"
#include "CalibrationInstrument.h"
#include "Maths/LeastSquaresProblem.h"
#include "FlexYCFCloneLookup.h"

using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{
 
    LeastSquaresResiduals::LeastSquaresResiduals(const BaseModelPtr baseModel):
        m_baseModel(baseModel),
        m_instrumentResiduals(baseModel),
        m_extraResiduals(baseModel)
    {
    }

	LeastSquaresResiduals::LeastSquaresResiduals(BaseModel* const model):
		m_baseModel(BaseModelPtr(model, NullDeleter())),
		m_instrumentResiduals(BaseModelPtr(model, NullDeleter())),
		m_extraResiduals(BaseModelPtr(model, NullDeleter()))
	{
	}


    void LeastSquaresResiduals::addInstrumentResidual(const CalibrationInstrumentPtr instrument,
                                                      const double weight)
    {
        m_instrumentResiduals.addInstrumentResidual(instrument, weight);
    }

    void LeastSquaresResiduals::addInstrumentResiduals(const CalibrationInstruments& instruments, 
                                                       const double weight)
    {
        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
            addInstrumentResidual(*iter, weight);
        }
    }

    void LeastSquaresResiduals::addExtraResidual(const ExtraResidualPtr extraResidual)
    {
        m_extraResiduals.addExtraResidual(extraResidual);
    }
        
	size_t LeastSquaresResiduals::sizeOfInstrumentResidual() const
	{
		 return m_instrumentResiduals.size();
	}

	InstrumentResidualPtr LeastSquaresResiduals::getInstrumentResidualAt( const size_t index ) const
	{        
		return m_instrumentResiduals[index];
	}

	size_t LeastSquaresResiduals::size() const
    {
        return sizeOfInstrumentResidual() + m_extraResiduals.size();
    }

	WeightedResidualPtr LeastSquaresResiduals::operator[](const size_t index) const
	{
        return (index < m_instrumentResiduals.size() ?
			WeightedResidualPtr(std::tr1::dynamic_pointer_cast<WeightedResidual>(m_instrumentResiduals[index])) :
			WeightedResidualPtr(std::tr1::dynamic_pointer_cast<WeightedResidual>(m_extraResiduals[index - m_instrumentResiduals.size()])));
	}


	double LeastSquaresResiduals::evaluate(const size_t index) const
    {
        checkIndex(index);

        return (index < m_instrumentResiduals.size() ?
            m_instrumentResiduals.getResidual(index) :
            m_extraResiduals.getResidual(index - m_instrumentResiduals.size()));
    }

    void LeastSquaresResiduals::computeGradient(const size_t index, Gradient& gradient) const
    {
        checkIndex(index);

        if(index < m_instrumentResiduals.size())
        {
            m_instrumentResiduals.computeGradient(index, gradient);
        }
        else
        {
            m_extraResiduals.computeGradient(index - m_instrumentResiduals.size(), gradient);
        }
    }

	void LeastSquaresResiduals::computeGradient(const size_t index, Gradient& gradient, const CurveTypeConstPtr& curveType) const
	{
		checkIndex(index);

		if(index < m_instrumentResiduals.size())
		{
			m_instrumentResiduals.computeGradient(index, gradient, curveType);
		}
		else
		{
			m_extraResiduals.computeGradient(index - m_instrumentResiduals.size(), gradient, curveType);
		}
	}


    void LeastSquaresResiduals::update()
    {   
        m_baseModel->update();
        m_instrumentResiduals.update();
        m_extraResiduals.update();
    }

    void LeastSquaresResiduals::finishCalibration()
    {
        for_each(m_instrumentResiduals.begin(),m_instrumentResiduals.end(),[&](const InstrumentResidualPtr& instr)
        {if (instr->getInstrument()) instr->getInstrument()->finishCalibration(m_baseModel);}  );
        //extra residuals do not hold instruments
    }

	LTQuant::LeastSquaresProblemPtr LeastSquaresResiduals::createLeastSquaresProblem() //const
	{
		return LeastSquaresProblemPtr(
			new LeastSquaresProblem(size(),
									[this] (size_t index) {return evaluate(index);},
									[this] () {update();},
									[this] (size_t index, Gradient& gradient) {computeGradient(index, gradient);}));
	}

	void LeastSquaresResiduals::clear()
	{
		m_instrumentResiduals.clear();
		m_extraResiduals.clear();
	}

    void LeastSquaresResiduals::checkIndex(const size_t index) const
    {
        if(index < 0 || index >= m_instrumentResiduals.size() + m_extraResiduals.size())
        {
            LT_THROW_ERROR( "Error of index in LeastSquaresResiduals." );
        }
    }

    std::ostream& LeastSquaresResiduals::print(std::ostream& out) const
    {
        out << m_instrumentResiduals << std::endl;

        // could print extra residuals as well
        return out;
    }

    /**
        @brief Clone this instance.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param lookup The previously created clones.
        @return       A clone of this instance.
    */
    ICloneLookupPtr LeastSquaresResiduals::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new LeastSquaresResiduals(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to preserve directed graph relationships in the copy.

        @param original The instance to copy.
        @param lookup   The lookup of previously created clones.
    */
    LeastSquaresResiduals::LeastSquaresResiduals(LeastSquaresResiduals const& original, CloneLookup& lookup) :
        m_baseModel(lookup.get(original.m_baseModel)),
        m_instrumentResiduals(original.m_instrumentResiduals, lookup),
        m_extraResiduals(original.m_extraResiduals, lookup)
    {
    }

}