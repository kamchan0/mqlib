/*****************************************************************************

	InstrumentDelta

	Represents a single risk delta relative to an instrument.

    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __InstrumentDelta_H__
#define	__InstrumentDelta_H__


//	Standard
#include <functional>

//	LTQC
#include "Macros.h"
#include "EnumBase.h"
#include "LT/const_string.h"

//	FlexYCF
#include "FlexYcfUtils.h"


namespace LTQuant 
{ 
    class GDTblWrapper; 
    typedef GDTblWrapper GenericData; 
}


namespace FlexYCF
{
	
	LTQC_ENUM_DECLARE_BEGIN( DeltaType )
		Delta, /* Structure? */ TurnDelta, XCcySpreadDelta, TenorSpreadDelta, ILDelta
	LTQC_ENUM_DECLARE_END( DeltaType )


	LTQC_FWD_DECLARE_SMART_PTRS( RiskInstrument )

	class InstrumentDelta
	{
	public:
		InstrumentDelta() {}
		explicit InstrumentDelta(const RiskInstrument& riskInstrument,
								 const double deltaValue,
                                 const double hedgeRatio,
								 const DeltaType deltaType);

		//	Returns the name of the instrument
		inline const LT::Str& getInstrumentName() const
		{
			return m_instrumentName;
		}

		//	Returns the description of the instrument
		inline const LT::Str& getInstrumentDescription() const
		{
			return m_instrumentDescription;
		}

		//	Returns the instrument rate
		inline double getInstrumentRate() const
		{
			return m_instrumentRate;
		}
		
		//	Returns the value of the delta
		inline double getDeltaValue() const
		{
			return m_deltaValue;
		}
		
		//	Returns the type of the delta
		inline DeltaType getType() const
		{
			return m_deltaType;
		}

        //	Returns the hedge ratio
		inline double getHedgeRatio() const
		{
			return m_hedgeRatio;
		}
		
		void add(const InstrumentDelta& rhs)
		{
			if( compareInstruments(rhs) )
			{
				m_deltaValue += rhs.getDeltaValue();
				m_hedgeRatio += rhs.getHedgeRatio();
			}
			else
			{
				LTQC_THROW( LTQC::ModelQCException, "Cannot add deltas: different instruments.");
			}
		}
		
		bool compareInstruments(const InstrumentDelta& rhs) const
		{
			return m_instrumentName.compareCaseless(rhs.getInstrumentName()) == 0
				&& m_instrumentDescription.compareCaseless(rhs.getInstrumentDescription()) == 0
				&& m_instrumentRate == rhs.getInstrumentRate()
				&& m_deltaType == rhs.getType();
		}

	private:
		LT::Str		m_instrumentName;
		LT::Str		m_instrumentDescription;
		double		m_instrumentRate;
		double		m_deltaValue;
		DeltaType	m_deltaType;
        double      m_hedgeRatio;
	};


	class CachedDerivInstrument;

	//	Returns the delta type of a calibration instrument
	DeltaType getDeltaType(const CachedDerivInstrument& instrument);

	namespace details
	{
		//	A helper functor to filter instrument delta in buckets
		class InstrumentDeltaFilter: public std::unary_function<const InstrumentDelta&, bool>
		{
		public:
			explicit InstrumentDeltaFilter(const DeltaType deltaType):
				m_deltaType(deltaType)
			{
			}
		
			inline bool operator()(const InstrumentDelta& instrumentDelta) const
			{
				return (instrumentDelta.getType() != m_deltaType);
			}

		private:
			DeltaType	m_deltaType;
		};
	}

	DEFINE_VECTOR( InstrumentDelta )

	//	Returns a delta vector from the full delta vector and the specified delta type
	InstrumentDeltaVector filterFromFullDeltaVector(const InstrumentDeltaVector& fullDeltaVector,
													const DeltaType deltaType);

	//	Converts a vector of instrument deltas to a GenericData
	void toGenericData(const InstrumentDeltaVector& deltaVector,
					   LTQuant::GenericData& instrumentDeltaData,
                       std::function<const double(const size_t)> instrumentRateProvider);

	//	Converts an InstrumentDeltaVector to a LT::TablePtr
    LT::TablePtr toTable(const InstrumentDeltaVector& deltaVector,std::function<const double(const size_t)> instrumentRateProvider);
	LT::TablePtr toTable(const InstrumentDeltaVector& deltaVector);
    std::vector<double> toVector(LT::TablePtr riskTbl, const LT::Str& key);
	void mergeInstrumentDeltas(InstrumentDeltaVector& deltaVector1, const InstrumentDeltaVector& deltaVector2);
}

#endif	//	__InstrumentDelta_H__