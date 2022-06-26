/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_ITOOLKIT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_ITOOLKIT_H_INCLUDED


#include "Data/GenericData.h"
#include "UkpCurve.h"
#include "StraightLineInterpolation.h"
#include "FlatExtrapolationMethod.h"
#include "StraightLineExtrapolationMethod.h"
#include "ExtrapolationSpecs.h"
#include "BaseCurve.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"


namespace FlexYCF
{

	class IToolkit
	{
	public:
		virtual LTQuant::GenericDataPtr getModelParameters() const = 0;

		virtual std::string getInitializationName() const = 0;
		virtual LTQuant::GenericDataPtr getInitializationParameters() const = 0;
		virtual std::string getKppName() const = 0;
		virtual std::string getSolverName() const = 0;
		virtual LTQuant::GenericDataPtr getSolverParameters() const = 0;
		virtual std::string getBaseRate() const = 0;
		virtual LTQuant::GenericDataPtr getCurvesInterpolation() const = 0;
		// Hooks for inflation curve
		//	virtual void setStaticData(const LTQuant::GenericDataPtr& inflationCurveData) const { }
		//	virtual void setEventsData(const LTQuant::GenericDataPtr& inflationCurveData) const { }

	protected:
		LTQuant::GenericDataPtr m_data;
	};


	/*
	template<class L = FlatExtrapolationMethod<LeftExtrapSpec>,
			 class R = StraightLineExtrapolationMethod<RightExtrap> >
	struct IncompleteInterpolatedCurveData
	{
	
	};
	*/

	// Template helper functors to assist derived toolkits to building curve interpolation data structure
	template<class T = UkpCurve,
			 class I = StraightLineInterpolation>
	struct InterpolationCurveData
	{
		LTQuant::GenericDataPtr operator()()
		{
			LTQuant::GenericDataPtr interpolationCurveDetailsData(new LTQuant::GenericData("Interp Curve", 0));
			
			IDeA::inject(*interpolationCurveDetailsData, IDeA_KEY(INTERPOLATIONCURVEDETAILS, CURVETYPE), T::getName());
			IDeA::inject(*interpolationCurveDetailsData, IDeA_KEY(INTERPOLATIONCURVEDETAILS, INTERP), I::getName());

			return interpolationCurveDetailsData;
		}
	};

	template<class T = UkpCurve,
			 class I = StraightLineInterpolation,
			 class L = FlatExtrapolationMethod<LeftExtrapSpec>,
			 class R = StraightLineExtrapolationMethod<RightExtrapSpec> >
	struct InterpolatedCurveData
	{
		LTQuant::GenericDataPtr operator()(std::string currency, const std::string& spineCurveId)
		{
			LTQuant::GenericDataPtr interpolatedCurveData(new LTQuant::GenericData(currency.append(spineCurveId), 0));
			//LTQuant::GenericDataPtr interpolationCurveDetailsData(new LTQuant::GenericData("Interp Curve", 0));
			
			IDeA::inject(*interpolatedCurveData, IDeA_KEY(INTERPOLATIONDETAILS, CURVETYPE), FlexYCF::BaseCurve::getName());
			IDeA::inject(*interpolatedCurveData, IDeA_KEY(INTERPOLATIONDETAILS, INTERPOLATIONCURVEDETAILS), InterpolationCurveData<T, I>()());
				// interpolationCurveDetailsData);
			IDeA::inject(*interpolatedCurveData, IDeA_KEY(INTERPOLATIONDETAILS, LEFTEXTRAP), L::getName());
			IDeA::inject(*interpolatedCurveData, IDeA_KEY(INTERPOLATIONDETAILS, RIGHTEXTRAP), R::getName());

			//IDeA::inject(*interpolationCurveDetailsData, IDeA_KEY(INTERPOLATIONCURVEDETAILS, CURVETYPE), T::getName());
			//IDeA::inject(*interpolationCurveDetailsData, IDeA_KEY(INTERPOLATIONCURVEDETAILS, INTERP), I::getName());

			return interpolatedCurveData;
		}
	};



}

#endif //__LIBRARY_PRICERS_FLEXYCF_ITOOLKIT_H_INCLUDED