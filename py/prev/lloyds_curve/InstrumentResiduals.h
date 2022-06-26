/*****************************************************************************

    InstrumentResiduals

	Represents a collection of instrument residuals in a a least squares
	problem.
    
	@Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTRESIDUALS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTRESIDUALS_H_INCLUDED
#pragma once

//	FlexYCF
#include "ResidualCollection.h"
#include "Gradient.h"
#include "InstrumentResidual.h"
#include "ResidualsUtils.h"


namespace FlexYCF
{
    
    FWD_DECLARE_SMART_PTRS( BaseModel )
    FWD_DECLARE_SMART_PTRS( CalibrationInstrument )
    FWD_DECLARE_SMART_PTRS( InstrumentResidual )
    FWD_DECLARE_SMART_PTRS( CurveType )

    // Represents a collection of instrument residuals.
    class InstrumentResiduals : public ResidualCollection
    {
    private:
        typedef std::vector<InstrumentResidualPtr> InstrumentResidualContainer;

    public:
        explicit InstrumentResiduals(const BaseModelPtr baseModel);
        InstrumentResiduals(InstrumentResiduals const& original, CloneLookup& lookup);

		inline void setResidualRepresentationType(const LeastSquaresRepresentationType::Enum_t representationType)
		{
			for(InstrumentResidualContainer::const_iterator iter(m_instrumentResiduals.begin());
				iter != m_instrumentResiduals.end(); ++iter)
			{
				(*iter)->setRepresentationType(representationType);
			}
		}

        void addInstrumentResidual(const CalibrationInstrumentPtr instrument,
                                   const double weight);

        virtual void update();
    
        inline size_t size() const
        {
            return m_instrumentResiduals.size();
        }
		
		/// Returns the index-th residual
		InstrumentResidualPtr operator[](const size_t index) const
		{
			return m_instrumentResiduals[index];
		}
        
        virtual void computeGradient(const size_t index, Gradient& gradient) const;
		virtual void computeGradient(const size_t index, 
									 Gradient& gradient, 
									 const CurveTypeConstPtr& curveType) const;

		virtual void clear();

        typedef InstrumentResidualContainer::iterator iterator;
        typedef InstrumentResidualContainer::const_iterator const_iterator;

        iterator begin()
        {
            return m_instrumentResiduals.begin();
        }

        const_iterator begin() const
        {
            return m_instrumentResiduals.begin();
        }

        iterator end()
        {
            return m_instrumentResiduals.end();
        }

        const_iterator end() const
        {
            return m_instrumentResiduals.end();
        }

         /// Prints the InstrumentResiduals
        std::ostream& print(std::ostream& out) const;

    protected:
        double getResidualImpl(const size_t index) const;

    private:
        virtual void checkIndex(const size_t index) const;

        BaseModelPtr				m_baseModel;
        InstrumentResidualContainer m_instrumentResiduals;
    };

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const InstrumentResiduals& instrumentResiduals)
		{
			return instrumentResiduals.print(out);
		}
	}

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_INSTRUMENTRESIDUALS_H_INCLUDED