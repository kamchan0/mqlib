/*****************************************************************************
    
	InstrumentResiduals

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InstrumentResiduals.h"
#include "InstrumentResidual.h"
#include "BaseModel.h"
#include "CalibrationInstrument.h"
#include "FlexYCFCloneLookup.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{

    InstrumentResiduals::InstrumentResiduals(const BaseModelPtr baseModel):
        ResidualCollection(1.0),
        m_baseModel(baseModel)
    {
    }

    /**
        @brief Pseudo copy constructor that uses lookup.

        @param original The instance that needs copying.
        @param lookup   A lookup of previously created clones.
    */
    InstrumentResiduals::InstrumentResiduals(InstrumentResiduals const& original, CloneLookup& lookup) :
        ResidualCollection(original),
        m_baseModel(lookup.get(original.m_baseModel))
    {
        CloneLookupUtils::assign(original.m_instrumentResiduals, m_instrumentResiduals, lookup);
    }

    void InstrumentResiduals::addInstrumentResidual(const CalibrationInstrumentPtr instrument,
                                                    const double weight)
    {
        m_instrumentResiduals.push_back(InstrumentResidualPtr(new InstrumentResidual(m_baseModel, instrument, weight)));
    }

    void InstrumentResiduals::computeGradient(const size_t index, Gradient& gradient) const
    {
        m_instrumentResiduals[index]->computeGradient(gradient);
    }

	void InstrumentResiduals::computeGradient(const size_t index, Gradient& gradient, const CurveTypeConstPtr& curveType) const
	{
		m_instrumentResiduals[index]->computeGradient(gradient, curveType);
	}

    void InstrumentResiduals::update()
    {
        for(InstrumentResidualContainer::iterator iter(m_instrumentResiduals.begin()); 
            iter != m_instrumentResiduals.end(); ++iter)
        {
            (*iter)->update();
        }
    }

	void InstrumentResiduals::clear()
	{
		m_instrumentResiduals.clear();
	}

    std::ostream& InstrumentResiduals::print(std::ostream& out) const
    {
        for(InstrumentResidualContainer::const_iterator iter(m_instrumentResiduals.begin());
            iter != m_instrumentResiduals.end(); ++iter)
        {
            out << (*iter) << endl;
        }
        return out;
    }
        
    double InstrumentResiduals::getResidualImpl(const size_t index) const
    {
        return m_instrumentResiduals[index]->getValue();
    }

    void InstrumentResiduals::checkIndex(const size_t index) const
    {
        if(index < 0 || index >= m_instrumentResiduals.size())
        {
            LT_THROW_ERROR( "Invalid index in ResidualCollection::getResidual." );
        }
    }


}