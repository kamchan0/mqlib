/*****************************************************************************

    InstrumentResidual

	Represents a single instrument residual and its representation in
	a least squares problem.
    

    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTRESIDUAL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTRESIDUAL_H_INCLUDED
#pragma once
#include "WeightedResidual.h"
#include "Gradient.h"
#include "ResidualsUtils.h"


namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( CalibrationInstrument )
	FWD_DECLARE_SMART_PTRS( CurveType )

    /// Represents the (weighted) residual of an instrument
    /// 
    class InstrumentResidual : public WeightedResidual
    {
    public:
        explicit InstrumentResidual(const BaseModelPtr baseModel,
                                    const CalibrationInstrumentPtr instrument,
                                    const double weight = 1.0);
    
		inline void setRepresentationType(const LeastSquaresRepresentationType::Enum_t representationType)
		{
			m_representationType = representationType;
		}

        virtual void update();
        virtual void computeGradient(Gradient& gradient) const;
		virtual void computeGradient(Gradient& gradient, const CurveTypeConstPtr& curveType) const;

		inline const CalibrationInstrumentPtr& getInstrument() const
		{
			return m_instrument;
		}

        /// Prints the InstrumentResidual
        std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        InstrumentResidual(InstrumentResidual const& instance, CloneLookup& lookup);

    private:
        virtual double getValueImpl() const;

        const BaseModelPtr						m_baseModel;
        const CalibrationInstrumentPtr			m_instrument;
		LeastSquaresRepresentationType::Enum_t	m_representationType;
    };  // InstrumentResidual

    DECLARE_SMART_PTRS( InstrumentResidual )
    
	namespace
	{
		std::ostream& operator<<(std::ostream& out, const InstrumentResidualPtr instrumentResidual)
		{
			return instrumentResidual->print(out);
		}
	}
}   // FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_INSTRUMENTRESIDUAL_H_INCLUDED