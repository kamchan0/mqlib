/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_GENERICTOOLKIT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_GENERICTOOLKIT_H_INCLUDED


#include "LTQuantInitial.h"
#include "IToolkit.h"
#include "Data/GenericData.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"


namespace FlexYCF
{
	// A generic toolkit template class, that is specialized for Stripper 
	//	and MultiTenor model implementations
	template<class M>
	class GenericToolkit : public IToolkit
	{
	public:
		explicit GenericToolkit(const LTQuant::GenericDataPtr& curveDetailsData, const LTQuant::GenericDataPtr& instrumentListData):
			m_curveDetailsData(curveDetailsData),
            m_instrumentListData(instrumentListData)
		{
			static std::string defaultCurrency("GBP");
			m_currency = defaultCurrency;
			
			if(m_curveDetailsData)
			{
				IDeA::permissive_extract(*m_curveDetailsData, IDeA_KEY(CURVEDETAILS, ASSET), m_currency, defaultCurrency);
			}
		}

		virtual LTQuant::GenericDataPtr getModelParameters() const ;

		virtual LTQuant::GenericDataPtr getCurveParameters() const;
		virtual std::string getKppName() const;
		virtual std::string getInitializationName() const;
		virtual LTQuant::GenericDataPtr getInitializationParameters() const;
		virtual std::string getSolverName() const;
		virtual LTQuant::GenericDataPtr getSolverParameters() const;
		virtual std::string getBaseRate() const;
		virtual LTQuant::GenericDataPtr getCurvesInterpolation() const;
	
		//	virtual void setStaticData(const LTQuant::GenericDataPtr& inflationCurveData) const;
		//	virtual void setEventsData(const LTQuant::GenericDataPtr& inflationCurveData) const;

		inline const std::string& currency() const { return m_currency; }

	protected:
		LTQuant::GenericDataPtr m_curveDetailsData;
	    LTQuant::GenericDataPtr m_instrumentListData;
	private:	
		std::string m_currency;
	};

}


#endif	//	__LIBRARY_PRICERS_FLEXYCF_GENERICTOOLKIT_H_INCLUDED