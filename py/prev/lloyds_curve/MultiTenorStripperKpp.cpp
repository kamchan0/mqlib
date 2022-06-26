/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"

//	FlexYCF
#include "MultiTenorStripperKpp.h"
#include "MultiTenorModel.h"
#include "MultiTenorFormulationModel.h"
#include "MultiTenorOISModel.h"
#include "MultiTenorOISFundingModel.h"
#include "NullDeleter.h"
#include "CurveType.h"
#include "RelatedCurveTypes.h"
#include "CashInstrument.h"
#include "ForwardRateAgreement.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "TenorBasisSwap.h"
#include "OvernightIndexedSwap.h"
#include "OISBasisSwap.h"
#include "Data\GenericData.h"
#include "KnotPlacementUtils.h"
#include "TenorZeroRate.h"
#include "ZeroRate.h"
#include "ForwardZeroRate.h"

LTQC_ENUM_DEFINE_BEGIN(FlexYCF::TenorSurfacePrecedenceRule)
	LTQC_REGISTER_ENUM(		CashRates,					"CashRates"						);
	LTQC_REGISTER_ALIAS(	CashRates,					"Cash Rates"					);
	LTQC_REGISTER_ALIAS(	CashRates,					"Cash"							);
	LTQC_REGISTER_ENUM(		TenorBasisSwaps,			"TenorBasisSwaps"				);
	LTQC_REGISTER_ALIAS(	TenorBasisSwaps,			"Tenor Basis Swaps"				);
	LTQC_REGISTER_ALIAS(	TenorBasisSwaps,			"TenorBasisSwap"				);
	LTQC_REGISTER_ALIAS(	TenorBasisSwaps,			"Tenor Basis Swap"				);
	LTQC_REGISTER_ALIAS(	TenorBasisSwaps,			"TenorBasis"					);
	LTQC_REGISTER_ALIAS(	TenorBasisSwaps,			"Tenor Basis"					);		
	LTQC_REGISTER_ENUM(		CashOnBase,					"CashOnBase"					);
	LTQC_REGISTER_ENUM(		CashAndTenorBasisSwaps,		"CashAndTenorBasisSwaps"		);
LTQC_ENUM_DEFINE_END(FlexYCF::TenorSurfacePrecedenceRule)


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    using namespace LTQuant;
    using namespace ModuleDate;
	
    namespace
    {
    
        bool tenorBasisAndLessOrEqualToTenor(const CalibrationInstrumentPtr& inst, 
                                             const CurveTypeConstPtr& /* baseRate */,
											 const CalibrationInstruments& instruments)
        {
            if(isOfType<TenorBasisSwap>(inst))
            {
                double years(getYearsBetween(inst->getStartDate(), inst->getEndDate()));
                CurveTypes relatedCurveTypes = RelatedCurveTypes::get(inst);
                CurveTypes::const_iterator typeIter(relatedCurveTypes.begin());
				if(*typeIter ==  CurveType::_3M() /*  *baseRate*/ )
                {
                    ++typeIter;
                }

				//	Note:
				//	- we do not remove tenor basis swaps if there is no cash instrument selected
                //	- we want to exclude any tenor basis swaps that start before 
				//		(or anywhere near) the corresponding cash rate
                if(cashInstrumentOfGivenTenorExists(instruments, *typeIter)
					&& years < 1.5 * (*typeIter)->getYearFraction())
                {
					LT_LOG << "Removing " << (*inst) << " from calibration instrument set." << std::endl;
                    return true;
                }

            }
            return false;
        }

      
    }

    MultiTenorStripperKpp::MultiTenorStripperKpp(const LT::date& valueDate,
                                                 const double flatInitialRate,
												 const TenorSurfacePrecedenceRule precedenceRule,
												 const bool multipleOnly) :
        m_valueDate(valueDate),
        m_flatInitialRate(flatInitialRate),
		m_precedenceRule(precedenceRule),
		m_multipleOnly(multipleOnly)
    {
    }

    MultiCurveKppPtr MultiTenorStripperKpp::createInstance(const LTQuant::GenericDataPtr masterTable)
    {
        const LTQuant::GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		LTQuant::GenericDataPtr modelParametersTable;
		const bool modelParamsFound(IDeA::permissive_extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE,FLEXYC_MODELPARAMETERS), modelParametersTable));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        // Retrieves the flat initial rate to initialise the curves with
        // use default = 5% if none is provided
        const double defaultFlatInitialRate(0.05);
        double flatInitialRate(defaultFlatInitialRate);
        
		if (modelParamsFound) 
		{
			LTQuant::GenericDataPtr solverParametersTable;
			const bool solverParamsFound(IDeA::permissive_extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, SOLVERPARAMETERS), solverParametersTable));
			if(solverParamsFound)
			{
				solverParametersTable->permissive_get<double>("Init Rate", 0, flatInitialRate, defaultFlatInitialRate);
			}
		}

		//	By default, cash rates take precedence on the (maturity/tenor) diagonal of the surface
		//	and discard tenor basis swaps on top right
		TenorSurfacePrecedenceRule precedenceRule(TenorSurfacePrecedenceRule::CashRates);
		bool multipleOnly(false);

		//	Extract knot placement parameters
		LTQuant::GenericDataPtr kppParamsData;
		if(IDeA::permissive_extract<LTQuant::GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, KPP_PARAMETERS), kppParamsData))
		{
			//	Try extract "Tenor Surface" parameter
			std::string tenorSurfaceRule;
			IDeA::permissive_extract<std::string>(*kppParamsData, IDeA_KEY(KPP_PARAMETERS, TENORSURFACE), tenorSurfaceRule);
			if(!tenorSurfaceRule.empty())
			{
				precedenceRule = TenorSurfacePrecedenceRule::fromString(tenorSurfaceRule);
			}

			//	Try extract "Multiple Only" parameter
			IDeA::permissive_extract<bool>(*kppParamsData, IDeA_KEY(KPP_PARAMETERS, MULTIPLEONLY), multipleOnly, false);
		}

        return MultiCurveKppPtr(new MultiTenorStripperKpp(valueDate, flatInitialRate, precedenceRule, multipleOnly));
    }

    void MultiTenorStripperKpp::selectInstruments(CalibrationInstruments& instruments, const BaseModelPtr baseModel)
    {
		const MultiTenorModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorModel>(baseModel));
		const MultiTenorFormulationModelPtr multiTenorFormulationModel(std::tr1::dynamic_pointer_cast<MultiTenorFormulationModel>(baseModel));
		if( multiTenorModel ||  multiTenorFormulationModel)
        {
			m_baseRate = multiTenorModel ? multiTenorModel->getBaseRate() : multiTenorFormulationModel->getBaseRate();
		}
		else
		{
			const MultiTenorOISModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorOISModel>(baseModel));
			if(multiTenorModel)
			{
				m_baseRate = multiTenorModel->getBaseRate();
			}
            else 
            {
			    const MultiTenorOISFundingModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorOISFundingModel>(baseModel));
                if(multiTenorModel)
			    {
				    m_baseRate = multiTenorModel->getBaseRate();
			    }
			    else
			    {
				    LT_THROW_ERROR( "Could not cast the BaseModel pointer to a MultiTenorModel/MultiTenorOISModel/MultiTenorOISFundingModel pointer in MultiTenorKpp::createKnotPoints(...)" );
			    }
            }
        }

        
        
        m_lastFuturesEndDate = getLastFuturesEndDate(m_valueDate, instruments);

        const bool selectFutures = m_lastFuturesEndDate > m_valueDate;
        // Remove FRAs if there are non-expired futures
        if( selectFutures )
        {
            removeFRAs( instruments );
        }
        else
        {
            // Remove FRAs that are not on the base rate
            removeNonBaseRateFRAs(m_baseRate, instruments);
        }
        m_lastFRAEndDate = getLastFRAEndDate(m_valueDate, instruments);

		//	Remove fixed futures
		removeFixedFutures(m_valueDate, instruments);
		
		//	Cash Instruments selection
		switch (m_precedenceRule)
		{
		case TenorSurfacePrecedenceRule::CashRates:
		{
			// get the list of tenors that we have basis swap information
			set<CurveTypeConstPtr> spreadTypes;
			fillSpreadTypes(spreadTypes, instruments);

			erase_if(instruments, [&spreadTypes, this] (const CalibrationInstrumentPtr& inst) {return cashAndNotOneOfRequiredTenors(inst, spreadTypes, m_baseRate);});
		}
		break;
		case TenorSurfacePrecedenceRule::TenorBasisSwaps:
		{
			//	Remove all non-base cash rates
			removeNonBaseCashRates(m_baseRate, instruments);
		}
		break;
		case TenorSurfacePrecedenceRule::CashOnBase:
	    case TenorSurfacePrecedenceRule::CashAndTenorBasisSwaps:
		{
			//	keep all cash
		}
		break;
		default:
			LTQC_THROW(IDeA::MarketException, "Invalid tenor basis precedence rule " << m_precedenceRule.getLabel());
			break;
		}

		//	Interest Rate Swaps selection
		erase_if(instruments, [this, selectFutures] (const CalibrationInstrumentPtr& inst) {return swapAndLessThanFuturesEndDate(inst, selectFutures ? m_lastFuturesEndDate : m_lastFRAEndDate);});
		 
		LT::date oisEndDate = getLastOISEndDate(m_valueDate, instruments);
		erase_if(instruments, [this, &oisEndDate] (const CalibrationInstrumentPtr& inst) {return OISBasisSwapAndLessThanOISEndDate(inst, oisEndDate);});


		//	Tenor Basis Swaps selection
        removeTenorBasisSwapsWithoutBasis( instruments );
		switch (m_precedenceRule)
		{
		case TenorSurfacePrecedenceRule::CashRates:
		{
			erase_if(instruments, [this, &instruments] (const CalibrationInstrumentPtr& inst) {return tenorBasisAndLessOrEqualToTenor(inst, m_baseRate, instruments);});
		} 
		break;
		case TenorSurfacePrecedenceRule::TenorBasisSwaps:
		case TenorSurfacePrecedenceRule::CashOnBase:
	    case TenorSurfacePrecedenceRule::CashAndTenorBasisSwaps:
		{
			//	Keep them all => do nothing!
		}
		break;
		default:
			LTQC_THROW(IDeA::MarketException, "Invalid tenor basis precedence rule " << m_precedenceRule.getLabel());
			break;
		}

		if (m_multipleOnly)
		{
			//	Remove all tenor basis whose maturity is less than the tenor of the other leg
			erase_if(instruments, [] (const CalibrationInstrumentPtr& inst) {return tenorBasisAndMaturityLessThanOtherLegTenor(inst);});
			//	.. and those whose maturity is not a multiple of the other leg tenor
			erase_if(instruments, [] (const CalibrationInstrumentPtr& inst) {return tenorBasisAndMaturityNotAMultipleOfOtherLegTenor(inst);});
		}
		
		// remove instruments with the idential end dates
		removeDuplicateCashInstruments(instruments);
		removeDuplicateCrossCurrencySwaps(instruments);
		if(!m_multipleOnly)
			removeDuplicateTenorBasisSwaps(instruments,4);
    }

    // to build up the stripper knot points first we build up a list of tenors that we have basis
    // swap information for
    bool MultiTenorStripperKpp::createKnotPoints(const CalibrationInstruments& instruments, 
                                                 const BaseModelPtr baseModel)
    {
        double initRate;  // used to fill the base rate or spread curves  
        const LT::date futuresOrFRAStartDate( max( getFirstFuturesStartDate(m_valueDate, instruments), getFirstFRAStartDate(m_valueDate, instruments) ) );

		LT_LOG << "MultiTenorStripper Kpp Creation - base rate: " << m_baseRate << endl;

        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
            double dateFlow;
            CurveTypeConstPtr knotPointTenor;
            // curve types to which we should add a point for this instrument's related curve types
            CurveTypes relatedCurveTypes = RelatedCurveTypes::get(*iter);
            CurveTypes::const_iterator typeIter(relatedCurveTypes.begin());
        
			// This part needs to be cleaned
            if(relatedCurveTypes.size() == 1)
            {
                // remember the cash instrument with same tenor as the model's
                // need to know the first futures before processing
				CashInstrumentPtr tmpCashInstrument = std::tr1::dynamic_pointer_cast<CashInstrument>(*iter);
                // change the knot point date on the cash rate with the same tenor as the model 
                // we are constructing if we have a valid first future date
                if(tmpCashInstrument)
                {
                    if(*(*typeIter) == *m_baseRate && futuresOrFRAStartDate > m_valueDate)
                    {                
                        dateFlow = getYearsBetween(m_valueDate, futuresOrFRAStartDate);
                        knotPointTenor = m_baseRate;
                    }
                    else
                    {
	                    dateFlow = getYearsBetween(m_valueDate, (*iter)->getEndDate());
 						switch (m_precedenceRule)
						{
						case TenorSurfacePrecedenceRule::CashRates:
						case TenorSurfacePrecedenceRule::TenorBasisSwaps: 
					    case TenorSurfacePrecedenceRule::CashAndTenorBasisSwaps: 
							knotPointTenor = *typeIter;
							break;
						case TenorSurfacePrecedenceRule::CashOnBase: 
							knotPointTenor = m_baseRate; 
							break;
						default:
							LTQC_THROW(IDeA::MarketException, "Invalid tenor basis precedence rule " << m_precedenceRule.getLabel());
							break;
						}
                        
                    }
                }
                else
                {
                    // all other primary instrument go in as on the base curve
					//dateFlow = getYearsBetween(m_valueDate, (*iter)->getEndDate());
                    //knotPointTenor = m_baseRate;
                
					dateFlow = getYearsBetween(m_valueDate, (*iter)->getEndDate());
					if(isOfType<Futures>(*iter))
					{
						// place Futures on the 3M (spread) curve
						knotPointTenor = CurveType::_3M();
					}
                    else if( isOfType<ForwardRateAgreement>(*iter) )
                    {
                        // place FRAs on their respective spread curve
						knotPointTenor = std::tr1::dynamic_pointer_cast<ForwardRateAgreement>(*iter)->getCurveType( );
                    }
					else if( isOfType<TenorZeroRate>(*iter) )
                    {
						knotPointTenor = std::tr1::dynamic_pointer_cast<TenorZeroRate>(*iter)->getCurveType( );
                    }
					else if( isOfType<ZeroRate>(*iter) )
                    {
						knotPointTenor = CurveType::Discount();
                    }
					else if( isOfType<ForwardZeroRate>(*iter) )
					{
						knotPointTenor = CurveType::Discount();
					}
					else 
					{
						InterestRateSwapPtr irs(std::tr1::dynamic_pointer_cast<InterestRateSwap>(*iter));
						if(irs)
						{
							// place IRS on the same tenor curve as its floating leg:
							//	knotPointTenor = CurveType::getFromYearFraction(irs->getFixedLegTenor()); 
							
							
							if(m_baseRate == CurveType::Discount())
							{
								knotPointTenor = CurveType::getFromYearFraction(irs->getFloatingLegTenor());
							}
							else
							{	// place IRS on the base rate curve:
								knotPointTenor = m_baseRate;
							}

						}
						else
						{
							LT_THROW_ERROR(" Error when placing the knots of Multi-Tenor Stripper Model");
						}
					}
				}
            }
            else if(relatedCurveTypes.size() == 2)
            {
                dateFlow = getYearsBetween(m_valueDate, (*iter)->getEndDate());
                
				// get the non-3M curve type
				if(*typeIter == CurveType::_3M())
				{
					++typeIter;
				}
				
				if(m_baseRate == CurveType::_3M() || !isOfType<TenorBasisSwap>(*iter))
				{
					// if the natural swap tenor is 3M or the instrument is not tenor basis swap
					//	place the knot on the non-3M curve type
					knotPointTenor = *typeIter;
				}
				else
				{
					// natural swap tenor != 3M and tenor basis swap 
					// place the knot on the non-3M curve if it's not the natural
					// swap tenor. Otherwise, just place at 3M.
					// knotPointTenor = (*typeIter == m_baseRate? CurveType::_3M() : *typeIter);

					if(*typeIter == m_baseRate && (*iter)->getEndDate() > max( m_lastFuturesEndDate, m_lastFRAEndDate ))
					{
						// the tenor is the natural swaps tenor
						// place the knot of the 3M curve if its end date
						// is after the last futures or FRA end date
						knotPointTenor = CurveType::_3M();
					}
					else
					{
						// otherwise (i.e. the tenor is not the natural swaps tenor,
						//	or the end date of the basis swap is before the last futures end date)
						// so place a knot on the curve of this tenor
						knotPointTenor = *typeIter;
					}
				}
            }
            else
            {
                LT_THROW_ERROR( "Unexpected number of related curves" );
            }
            
            // Initialises the base rate with the flat initial rate (times date-flow as
            //  a logFVF formulation is taken for the model) and the spread curves with 0
            initRate = (*knotPointTenor == *m_baseRate ? dateFlow * m_flatInitialRate : 0.0);

			// dump the knot-points to the model, using an initial flat rate
			const MultiTenorModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorModel>(baseModel));
			const MultiTenorFormulationModelPtr multiTenorFormulationModel(std::tr1::dynamic_pointer_cast<MultiTenorFormulationModel>(baseModel));
            if(multiTenorModel)
			{
                if( isOfType<OvernightIndexedSwap>(*iter) || isOfType<OISBasisSwap>(*iter) )
                {
                   LT_THROW_ERROR( "MutltTenor model can not deal with OIS instruments, use MultiTenorOIS or MultiTenorOISFunding model." );
                }
				multiTenorModel->addKnotPoint(knotPointTenor, KnotPoint(dateFlow, initRate, false, *iter));
			}
			else if(multiTenorFormulationModel)
			{
				multiTenorFormulationModel->addKnotPoint(knotPointTenor, KnotPoint(dateFlow, initRate, false, *iter));
			}
			else
			{
				const MultiTenorOISModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorOISModel>(baseModel));
                if(multiTenorModel)
                {
				    multiTenorModel->addKnotPoint(knotPointTenor, KnotPoint(dateFlow, initRate, false, *iter));
                }
                else
                {

				    const MultiTenorOISFundingModelPtr multiTenorModel(std::tr1::dynamic_pointer_cast<MultiTenorOISFundingModel>(baseModel));
				    multiTenorModel->addKnotPoint(knotPointTenor, KnotPoint(dateFlow, initRate, false, *iter));
                }
			}
            LT_LOG << /*(*iter)->getType() << (*iter)->getDescription()*/(*iter) << " \t -> \t "  
                << "(" << dateFlow << ", " << initRate << ")\t on \t" << knotPointTenor->getDescription() << endl;

        }

        return true;
    }

    // for each instrument, look for tenor basis instruments
    // which have 2 curve types neither of which is discounting
    // and add the curve types referenced
    void MultiTenorStripperKpp::fillSpreadTypes(set<CurveTypeConstPtr>& spreadTypes, const CalibrationInstruments& instruments)
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

}   //  FlexYCF
