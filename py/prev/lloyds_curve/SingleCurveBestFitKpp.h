/*****************************************************************************

    @Originator	
    
    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEBESTFITKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SINGLECURVEBESTFITKPP_H_INCLUDED
#pragma once

#include "SingleCurveKpp.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( SingleCurveBestFitKpp )

    class SingleCurveBestFitKpp : public SingleCurveKpp
    {
	private:
		typedef std::map<CalibrationInstrumentPtr, double> WeightedInstrumentsMap;

    public:
        static std::string getName();

		SingleCurveBestFitKpp(	const LTQuant::GenericDataPtr& knotPointInstrumentTbl, 
								const LTQuant::GenericDataPtr& extraKpTbl, 
								const LTQuant::GenericDataPtr& weightedInstrumentTbl,
								const BaseKnotPointPlacementPtr& nestedKpp);

        /// Creates a SingleCurveBestFitKpp
        static SingleCurveBestFitKppPtr createInstance(const LTQuant::GenericDataPtr masterTable);

		virtual void selectInstruments(CalibrationInstruments& instruments, 
                                       const BaseModelPtr baseModel);
        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel);
		virtual void onLeastSquaresResidualsAdded(LeastSquaresResiduals& lsr) const;

	private:
		// table from kpp params for instruments associated with kp
		LTQuant::GenericDataPtr m_knotPointInstrumentTbl;
		// table from kpp params for extra knot points
		LTQuant::GenericDataPtr m_extraKpTbl;
		// table from kpp params for instruments with specified weight
		LTQuant::GenericDataPtr m_weightedInstrumentTbl;
		// map of instruments and the specified weights, only those survive the selection process
		WeightedInstrumentsMap m_weightedInstruments;

		BaseKnotPointPlacementPtr m_nestedKpp;

    };   //  SingleCurveBestFitKpp

	DECLARE_SMART_PTRS( SingleCurveBestFitKpp )

	template<class Model>
	void addKnotPoints( const Model& model, 
						const CalibrationInstruments& keyInstruments, 
						const std::vector<std::pair<double,double> >& extraKp,
						const BaseKnotPointPlacementPtr& nestedKpp)
	{
		if (nestedKpp)
			nestedKpp->createKnotPoints(keyInstruments, model);
		else
		{
			const double defaultInitRate(0.05);
			for (CalibrationInstruments::const_iterator iter(keyInstruments.begin()); iter != keyInstruments.end(); ++iter)
			{
				const double endDateFlow(ModuleDate::getYearsBetweenAllowNegative(model->getValueDate(), (*iter)->getEndDate()));
				model->addKnotPoint(KnotPoint(endDateFlow, endDateFlow * defaultInitRate, false, *iter));
			}
		}

		// add extra kp
		for_each(extraKp.begin(),extraKp.end(), [&model](const pair<double,double>& kp){model->addKnotPoint(KnotPoint(kp.first, kp.second, false));});
	}

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_SINGLECURVEBESTFITKPP_H_INCLUDED