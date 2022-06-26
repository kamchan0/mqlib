/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENTS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENTS_H_INCLUDED
#pragma once

#include "CalibrationInstrument.h"

namespace Data
{
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}


namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( BaseModel )

    /// CalibrationInstruments represents a collection of
    /// instruments used to calibrate a model.
    class CalibrationInstruments : public ICloneLookup
    {
    protected:
        typedef std::vector<CalibrationInstrumentPtr> InstrumentContainer;

    public:
        typedef InstrumentContainer::iterator          iterator;
        typedef InstrumentContainer::const_iterator    const_iterator;
        
        inline iterator begin()
        {
            return m_instruments.begin();
        }
        inline const_iterator begin() const
        {
            return m_instruments.begin();
        }
        inline iterator end()
        {
            return m_instruments.end();
        }
        inline const_iterator end() const
        {
            return m_instruments.end();
        }

        /// Erase all instruments in the [first, last) range specified 
        inline void erase(const iterator first, const iterator last)
        {
            m_instruments.erase(first, last);
        }

        // Returns the number of instruments
        inline size_t size() const
        {
            return m_instruments.size();
        }

        // Returns the index-th instrument in the container (1st instrument at 0)
        inline CalibrationInstrumentPtr operator[](const size_t index) const
        {
            return m_instruments[index];
        }

		//	Clears the collection of instruments
		inline void clear()
		{
			m_instruments.clear();
		}

        /// Adds an instrument to the instance
        void add(const CalibrationInstrumentPtr& calibrationInstrument);

        /// Returns whether the current calibration instruments contain
        ///  at least one instrument of a certain type, e.g. Cash or IRS
        template<class T>
        const_iterator find() const
        {
            return find_if(m_instruments.begin(), m_instruments.end(), FlexYCF::isOfType<T>);
        }

        /// Returns whether the current calibration instruments contain
        ///  at least one instrument of a certain type, e.g. Cash or IRS
        template<class T>
        bool has() const
        {
            return (find_if(m_instruments.begin(), m_instruments.end(), FlexYCF::isOfType<T>) != m_instruments.end());
        }
        
		/// Returns the index-th residual
		double getResidual(const BaseModelPtr& model,
						   const size_t index) const;

		// Calculate residual gradient RELATIVE TO MARKET PRICES
		//	so that in practice, given how an instrument residual is calculated,
		//	gradients have the form: (0, ..., 0, -1, 0, ..., 0)
		//	with a -1 at the index-th position (starting from position 0)
		void computeResidualGradient(const BaseModelPtr& model,
									 const size_t index,
									 std::vector<double>& gradient) const;

        /// Returns the residuals norm in the specified model.
        double getResidualsNorm(const BaseModelPtr model) const;

		/// Fill the vector of instruments with
		/// the instruments of the current instance of the specified type 
		void extractInstrumentsOfType(const std::string& instrumentTypeName, std::vector<CalibrationInstrumentPtr>& instruments) const;

		template<class T>
		void extractInstrumentsOfType(std::vector<CalibrationInstrumentPtr>& instruments) const
		{
			extractInstrumentsOfType(T::getName(), instruments);
		}

		InstrumentContainer  getInstrumentCollection() const
		{
			return m_instruments;
		}

		//	Sets all the instruments as placed or not according to
		//	the specified boolean
		inline void setAsPlaced(const bool wasPlaced)
		{
			for(InstrumentContainer::iterator iter(m_instruments.begin()); iter != m_instruments.end(); ++iter)
			{
				(*iter)->setAsPlaced(wasPlaced);
			}
		}

		LTQuant::GenericDataPtr asGenericData() const;

        std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
        void assign(CalibrationInstruments const& from, CloneLookup& lookup);

    private:
		template<class T>
		void tryToAddInstrumentTable(const LTQuant::GenericDataPtr& instrumentsListData) const;

		template<class T>
		static void iterateAndFillData(const LTQuant::GenericDataPtr& instrumentListData,
									   const CalibrationInstruments::InstrumentContainer& instruments);

		template<class T>
		static void fillData(const LTQuant::GenericDataPtr& instrumentListData, 
							 const T& instrument,
							 const size_t index);

        InstrumentContainer m_instruments;
    };  //  CalibrationInstruments

    DECLARE_SMART_PTRS( CalibrationInstruments )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const CalibrationInstruments& calibrationInstruments)
		{
			return calibrationInstruments.print(out);
		}
	}

	template<class T>
	void CalibrationInstruments::tryToAddInstrumentTable(const LTQuant::GenericDataPtr& instrumentsListData) const
	{
		InstrumentContainer instruments;

		extractInstrumentsOfType<T>(instruments);
		
		if(!instruments.empty())
		{
			LTQuant::GenericDataPtr instrumentListData(new LTQuant::GenericData(T::getName(), 0));
			instrumentsListData->set<LTQuant::GenericDataPtr>(T::getName(), 0, instrumentListData);

			iterateAndFillData<T>(instrumentListData, instruments);
		}	
	}

	template<class T>
	void CalibrationInstruments::iterateAndFillData(const LTQuant::GenericDataPtr& instrumentListData,
													const CalibrationInstruments::InstrumentContainer& instruments)
	{
		for(size_t index(0); index < instruments.size(); ++ index)
		{
			fillData<T>(instrumentListData, dynamic_cast<T&>(*instruments[index]), index);
		}	
	}


	
}   // FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENTS_H_INCLUDED