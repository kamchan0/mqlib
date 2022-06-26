/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "NaturalSwapTenorKpp.h"
#include "CashInstrument.h"
#include "MultiTenorModel.h"
#include "NullDeleter.h"
#include "CurveType.h"
#include "RelatedCurveTypes.h"
#include "CashInstrument.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "OvernightIndexedSwap.h"
#include "OISBasisSwap.h"
#include "TenorBasisSwap.h"
#include "Data\GenericData.h"
#include "CrossCurrencySwap.h"
#include "KnotPoint.h"
#include "KnotPlacementUtils.h"

// IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"

using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{

	namespace
	{
		
		struct SameTenorAs
		{
			SameTenorAs(const CurveTypeConstPtr& tenor_):
				tenor(tenor_)
			{
			}

			// Does the instrument include the tenor of the functor as the related curve type?
			bool operator()(const CalibrationInstrumentPtr& instrument) const
			{
				const CurveTypes curveTypes(RelatedCurveTypes::get(instrument));
				return (find(curveTypes.begin(), curveTypes.end(), tenor) != curveTypes.end()); 
			}
			CurveTypeConstPtr tenor;
		};
        
        bool tenorBasisAndLessOrEqualToTenor(const CalibrationInstrumentPtr& tenorBasisSwap, 
											 const CurveTypeConstPtr& baseRate,
											 std::vector<CalibrationInstrumentPtr>& cashInstruments)
        {
			// Note: tenor basis swaps points are placed on the tenor curve corresponding
			//	to their non-3M leg, unless the latter is the swap tenor, in which case
			//	the points are placed on the 3M curve.
			// So there should never be a TBS point on the base rate curve
            if(isOfType<TenorBasisSwap>(tenorBasisSwap))
            {
                const CurveTypes basisCurveTypes(RelatedCurveTypes::get(tenorBasisSwap));
                // CurveTypes::const_iterator typeIter;

				// find the 1st non-3M curve type
				size_t cnt;
				for(cnt = 0; cnt < basisCurveTypes.size() && basisCurveTypes[cnt] == CurveType::_3M(); ++cnt)
					;	// skip
				
				if(cnt == basisCurveTypes.size())
				{
					LT_THROW_ERROR( "No non-3m leg" );
				}
				const CurveTypeConstPtr basisTenor(basisCurveTypes[cnt] == baseRate? CurveType::_3M(): basisCurveTypes[cnt]);
				
				// is there a cash instrument of the same tenor as the basis swap?
				// We assume that cash instruments are placed on the tenor curve that 
				//	correspond to their maturity
				SameTenorAs sameTenorAs(basisTenor);
				std::vector<CalibrationInstrumentPtr>::const_iterator iter(find_if(cashInstruments.begin(), cashInstruments.end(), sameTenorAs));

				const double years(ModuleDate::getYearsBetween(tenorBasisSwap->getStartDate(), tenorBasisSwap->getEndDate()));

				// we want to exclude any tenor basis swaps that start before (or anywhere near)
				//	a corresponding cash rate that is going to be used to place a knot
				if(cnt != basisCurveTypes.size() && ModuleDate::getYearsBetween(tenorBasisSwap->getStartDate(), tenorBasisSwap->getEndDate()) < 1.5 * basisTenor->getYearFraction())
				{
					LT_LOG << "Removing " << (*tenorBasisSwap) << " from calibration instrument set." << std::endl;
					return true;
				}
				else
				{
					LT_LOG << (*tenorBasisSwap) << "(" << years << ") not removed" << std::endl;;
				}
				
            }
            return false;
        }
		
		// for each instrument, look for tenor basis instruments
		// which have 2 curve types neither of which is discounting
		// and add the curve types referenced
		void fillSpreadTypes(set<CurveTypeConstPtr>& spreadTypes, const CalibrationInstruments& instruments)
		{
			for(CalibrationInstruments::const_iterator instrIter(instruments.begin()); instrIter != instruments.end(); ++instrIter)
			{
				CurveTypes relatedCurveTypes = RelatedCurveTypes::get(*instrIter);
	        
				if(relatedCurveTypes.size() == 2)
				{
					if(find(relatedCurveTypes.begin(), relatedCurveTypes.end(), CurveType::Discount()) == relatedCurveTypes.end())
					{
						CurveTypes::const_iterator typeIter(relatedCurveTypes.begin());
						// it's a Tenor Basis Swap
						spreadTypes.insert(*typeIter);
						++typeIter;
						spreadTypes.insert(*typeIter);
					}
				}
			}
		}
	}	// anonymous namespace

	NaturalSwapTenorKpp::NaturalSwapTenorKpp(const LT::date valueDate,
											 const CurveTypeConstPtr& naturalSwapTenor):
		m_valueDate(valueDate),
		m_naturalSwapTenor(naturalSwapTenor)
	{
	}

	std::string NaturalSwapTenorKpp::getName()
	{
		return "SwapTenorKpp";
	}
	
	MultiCurveKppPtr NaturalSwapTenorKpp::createInstance(const LTQuant::GenericDataPtr& masterTable)
	{
		const LTQuant::GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LTQuant::GenericDataPtr modelParamsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS)));

		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		const std::string naturalSwapTenorDescription(IDeA::extract<std::string>(*modelParamsTable, IDeA_KEY(FLEXYC_MODELPARAMETERS,BASERATE)));
		
		const CurveTypeConstPtr naturalSwapTenor(CurveType::getFromDescription(naturalSwapTenorDescription));
		
		return MultiCurveKppPtr(new NaturalSwapTenorKpp(valueDate, naturalSwapTenor));
	}

	void NaturalSwapTenorKpp::selectInstruments(CalibrationInstruments& instruments,
												const BaseModelPtr model)
	{
		const MultiTenorModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorModel>(model));
        if(!multiTenorModel)
        {
            LT_THROW_ERROR( "Could not cast the BaseModel pointer to a MultiTenorModel pointer in MultiTenorKpp::createKnotPoints(...)" );    
        }
		
		m_naturalSwapTenor = multiTenorModel->getBaseRate();

		// get the list of tenors that we have basis swap information
        set<CurveTypeConstPtr> spreadTypes;
        fillSpreadTypes(spreadTypes, instruments);

		std::vector<CalibrationInstrumentPtr> cashInstruments;

        const LT::date lastFuturesEndDate(getLastFuturesEndDate(m_valueDate, instruments));

		erase_if(instruments, [&model] (const CalibrationInstrumentPtr& inst) {return isFutureAndFixesBeforeBuildDate(inst, model->getValueDate());});
		erase_if(instruments, [&spreadTypes, this] (const CalibrationInstrumentPtr& inst) {return cashAndNotOneOfRequiredTenors(inst, spreadTypes, m_naturalSwapTenor);});
		erase_if(instruments, [&lastFuturesEndDate] (const CalibrationInstrumentPtr& inst) {return swapAndLessThanFuturesEndDate(inst, lastFuturesEndDate);});
        
		// get the remaining cash instruments
        instruments.extractInstrumentsOfType<CashInstrument>(cashInstruments);
        removeTenorBasisSwapsWithoutBasis( instruments );
		erase_if(instruments, [this, &cashInstruments] (const CalibrationInstrumentPtr& inst) 
			{return tenorBasisAndLessOrEqualToTenor(inst, m_naturalSwapTenor, cashInstruments);});
	}

	bool NaturalSwapTenorKpp::createKnotPoints(const CalibrationInstruments& instruments,
											   const BaseModelPtr model)
	{
		const MultiTenorModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorModel>(model));	
		if(!multiTenorModel)
		{
			LT_THROW_ERROR( "The specified model is not a MultiTenorModel" );
		}

		const double flatInitialRate(0.02);
		double dateFlow = -1.0;
		CurveTypeConstPtr knotPointTenor;
		CurveTypes::const_iterator typeIter;
		CurveTypes relatedCurveTypes;

		for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
		{
			if(isOfType<CashInstrument>(*iter))
			{
				knotPointTenor = RelatedCurveTypes::get(*iter)[0];
				if(knotPointTenor == m_naturalSwapTenor)
				{
					// place a point at the cash end date as usual?
					dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
					// place a point at the 1st futures start date?
					// place a point at the 2nd futures start date?
				}
				else
				{
					// add point at the end date of cash
					dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
				}
			}
			else if(isOfType<Futures>(*iter))
			{
				// place point at end date on natural swap tenor curve, except maybe for 1st and 2nd futures
				knotPointTenor = m_naturalSwapTenor;
				dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
			}
			else if(isOfType<InterestRateSwap>(*iter))
			{
				// place point at end date on natural swap tenor curve
				knotPointTenor = m_naturalSwapTenor;
				dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
			}
			else if(isOfType<CrossCurrencySwap>(*iter))
			{
				// place on funding curve
				knotPointTenor = CurveType::Discount();
				dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
			}
			else if(isOfType<OvernightIndexedSwap>(*iter))
			{
				// place on funding curve
				knotPointTenor = CurveType::Discount();
				dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
			}
            else if(isOfType<OISBasisSwap>(*iter))
			{
				// place on funding curve
				knotPointTenor = CurveType::Discount();
				dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
			}
			else if(isOfType<TenorBasisSwap>(*iter))
			{
				// place on non-3m curve, except if it's natural swap tenor curve
				relatedCurveTypes = RelatedCurveTypes::get(*iter);
				for(typeIter = relatedCurveTypes.begin(); 
					typeIter != relatedCurveTypes.end() && *typeIter == CurveType::_3M();
					++typeIter)
					;	// skip

				if(typeIter == relatedCurveTypes.end())
				{
					LT_THROW_ERROR( "No non-3m leg" );
				}
				dateFlow = ModuleDate::getYearsBetween(m_valueDate, (*iter)->getEndDate());
				knotPointTenor = (*typeIter == m_naturalSwapTenor? CurveType::_3M(): *typeIter);

			}
		
			// Initialises the base rate with the flat initial rate (times date-flow as
            //  a logFVF formulation is taken for the model) and the spread curves with 0
            const double initRate = (*knotPointTenor == *m_naturalSwapTenor ? dateFlow * flatInitialRate : 0.0);
            
			LT_LOG << (*iter) << " \t -> \t "  
                << "(" << dateFlow << ", " << initRate << ")\t on \t" << knotPointTenor->getDescription() << endl;

            multiTenorModel->addKnotPoint(knotPointTenor, KnotPoint(dateFlow, initRate, false, *iter));
		}

		return true;
	}
}