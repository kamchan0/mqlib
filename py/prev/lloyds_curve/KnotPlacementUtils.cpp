/*****************************************************************************
    
	This file contains the implementation of some knot-placement utility 
	functions
    
	
	@Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
//	FlexYCF
#include "KnotPlacementUtils.h"
#include "CalibrationInstruments.h"
#include "CashInstrument.h"
#include "ForwardRateAgreement.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "CrossCurrencySwap.h"
#include "TenorBasisSwap.h"
#include "BaseModel.h"
#include "StripperModel.h"
#include "CurveType.h"
#include "RelatedCurveTypes.h"
#include "ModuleDate/InternalInterface/DayCounterFactory.h"
#include "ModuleDate/InternalInterface/Utils.h"
#include "OISBasisSwap.h"
#include "OvernightIndexedSwap.h"

#include <set>

LTQC_ENUM_DEFINE_BEGIN(FlexYCF::CashSelection)
	    LTQC_REGISTER_ENUM(		BaseRate,					"BaseRate"						);
	    LTQC_REGISTER_ENUM(		FutureStartBlending,	    "FutureStartBlending"   	    );
        LTQC_REGISTER_ENUM(		FutureStartLinear,			"FutureStartLinear"       		);
LTQC_ENUM_DEFINE_END(FlexYCF::CashSelection)

using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{
    bool cashAndNotOneOfRequiredTenors(const CalibrationInstrumentPtr& inst, 
									   const set<CurveTypeConstPtr>& spreadTypes,
									   const CurveTypeConstPtr& baseRate)
    {
        if(isOfType<CashInstrument>(inst))
        {
            CurveTypes relatedCurveTypes = RelatedCurveTypes::get(inst);

            if(find(spreadTypes.begin(), spreadTypes.end(), *relatedCurveTypes.begin()) == spreadTypes.end()
				&& relatedCurveTypes[0] != baseRate)
            {
                // cash tenor not in list so delete
				LT_LOG << "Removing " << (*inst) << " from calibration instrument set." << std::endl;
                return true;
            } else {
                // cash tenor in list so keep
                return false;
            }
        }
        // not cash so keep
        return false;
    }

	bool cashAndNotBaseRate(const CalibrationInstrumentPtr& instrument,
							const CurveTypeConstPtr& baseRate)
	{
		return isOfType<CashInstrument>(instrument) && instrument->getDescription().compareCaseless( baseRate->getDescription() );
	}
    
    bool cashAndAfterGivenDate(const CalibrationInstrumentPtr& instrument, const LT::date& date)
	{
		return isOfType<CashInstrument>(instrument) && instrument->getEndDate() > date;
	}
	
    void removeNonBaseCashRates(const CurveTypeConstPtr& baseRate, CalibrationInstruments& instruments)
	{
		erase_if(instruments, [baseRate] (const CalibrationInstrumentPtr& c) {return cashAndNotBaseRate(c, baseRate);});
	}
    
    void removeCashRatesAfterGivenDate(const LT::date& date, CalibrationInstruments& instruments)
	{
		erase_if(instruments, [date] (const CalibrationInstrumentPtr& c) {return cashAndAfterGivenDate(c, date);});
	}

    bool fraAndNotBaseRate( const CalibrationInstrumentPtr& instrument,
							const CurveTypeConstPtr& baseRate)
	{
		return isOfType<ForwardRateAgreement>(instrument) && !CurveType::Equals( *baseRate, *( std::tr1::dynamic_pointer_cast<ForwardRateAgreement>(instrument)->getCurveType( ) ) );
	}

    void removeNonBaseRateFRAs(const CurveTypeConstPtr& baseRate, CalibrationInstruments& instruments)
    {
		erase_if(instruments, [baseRate] (const CalibrationInstrumentPtr& instrument) {return fraAndNotBaseRate(instrument, baseRate);});
    }

	// Function used to remove interest rate swaps whose end date is before the last futures end date
    bool swapAndLessThanFuturesEndDate(const CalibrationInstrumentPtr& inst, const LT::date& lastFuturesEndDate)
    {
		if(isOfType<InterestRateSwap>(inst) && inst->getEndDate() <= lastFuturesEndDate)
		{
			LT_LOG << "Removing " << (*inst) << " from calibration instrument set." << std::endl;
			return true;
		}
		// not IRS so keep
		return false;
	}

	bool OISBasisSwapAndLessThanOISEndDate(const CalibrationInstrumentPtr& inst, const LT::date& oisEndDate)
    {
		if(isOfType<OISBasisSwap>(inst) && inst->getEndDate() <= oisEndDate)
		{
			LT_LOG << "Removing " << (*inst) << " from calibration instrument set." << std::endl;
			return true;
		}
		return false;
	}
	
	bool IsCashFuturesFRASwap::operator()(const CalibrationInstrumentPtr& instrument) const
	{
		return isOfType<CashInstrument>(instrument) || isOfType<Futures>(instrument) || isOfType<ForwardRateAgreement>(instrument) || isOfType<InterestRateSwap>(instrument);
	}

	void removeNonCashFuturesFRASwaps(CalibrationInstruments& instruments)
	{
		erase_if(instruments, std::not1(IsCashFuturesFRASwap()));
	}

	bool isFutureAndFixesBeforeBuildDate(const CalibrationInstrumentPtr& instrument,
										 const LT::date& buildDate)
	{
		return (isOfType<Futures>(instrument) && instrument->getFixingDate() <= buildDate);
	}

	bool isCashAndHasTenor(const CalibrationInstrumentPtr& instrument, const LT::Str& description)
	{
		return (isOfType<CashInstrument>(instrument) && description.compareCaseless(instrument->getDescription()) == 0);
	}

	bool isCrossCurrencySwapAndHasTenor(const CalibrationInstrumentPtr& instrument, const LT::Str& description)
	{
		return (isOfType<CrossCurrencySwap>(instrument) && description.compareCaseless(instrument->getDescription()) == 0);
	}

	bool isTenorBasisSwapAndHasTenor(const CalibrationInstrumentPtr& instrument, const LT::Str& description)
	{
		return (isOfType<TenorBasisSwap>(instrument) && description.compareCaseless(instrument->getDescription()) == 0);
	}
	
	void removeFixedFutures(const LT::date& valueDate, CalibrationInstruments& instruments)
	{
		erase_if(instruments, [&valueDate] (const CalibrationInstrumentPtr& instrument) {return isFutureAndFixesBeforeBuildDate(instrument, valueDate);});
	}

    // Remove FRAs
	void removeFRAs(CalibrationInstruments& instruments)
    {
        erase_if(instruments, isOfType<ForwardRateAgreement> );
    }

    // Remove tenor basis swaps without basis, that is the ones where the reference and other legs have the same tenor
    void removeTenorBasisSwapsWithoutBasis(CalibrationInstruments& instruments)
    {
		erase_if(instruments, tenorBasisWithoutBasis);
    }

    bool tenorBasisWithoutBasis(const CalibrationInstrumentPtr& instrument)
    {
        return isOfType<TenorBasisSwap>(instrument) && CurveType::Equals( *( CurveType::getFromDescription( std::tr1::dynamic_pointer_cast<TenorBasisSwap>(instrument)->getReferenceTenor( ) ) ),
                                                                          *( CurveType::getFromDescription( std::tr1::dynamic_pointer_cast<TenorBasisSwap>(instrument)->getOtherTenor( ) ) ) );
    }

    // Returns whether the specified instrument is a tenor basis swaps of the specified maturity
    bool tenorBasisOfGivenMaturity(const CalibrationInstrumentPtr& instrument, const string& maturity)
    {
        return isOfType<TenorBasisSwap>(instrument) && (instrument->getDescription().compareCaseless( maturity ) == 0);
    }

	bool tenorBasisAndMaturityLessThanOtherLegTenor(const CalibrationInstrumentPtr& instrument)
	{
		const TenorBasisSwapPtr tenorBasisSwap(std::tr1::dynamic_pointer_cast<TenorBasisSwap>(instrument));
		return tenorBasisSwap && tenorBasisSwap->hasSyntheticOtherLeg();	
	}

	bool tenorBasisAndMaturityNotAMultipleOfOtherLegTenor(const CalibrationInstrumentPtr& instrument)
	{
		const TenorBasisSwapPtr tenorBasisSwap(std::tr1::dynamic_pointer_cast<TenorBasisSwap>(instrument));

		using namespace LTQuant;
        using namespace ModuleDate;

		//	Returns whether the maturity, as a number of months
		//	is not a multiple of the other tenor of the basis swap
		return tenorBasisSwap && (getMonthsBetween(tenorBasisSwap->getStartDate(), tenorBasisSwap->getEndDate()) % static_cast<long>(floor(dateDescToMonths(tenorBasisSwap->getOtherTenor()) + 0.5)) != 0);
	}

    LT::date getLastFuturesEndDate(const LT::date& valueDate, const CalibrationInstruments& instruments)
    {
        LT::date lastFuturesEndDate(valueDate);

        // initialize last futures end date ( could be stored in a member variable for reuse in createKnotPoints )
        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
            if(isOfType<Futures>(*iter) && (*iter)->getEndDate() > lastFuturesEndDate)
            {
                lastFuturesEndDate = (*iter)->getEndDate();
            }
        }
        return lastFuturesEndDate;
    }

    LT::date getFirstFuturesStartDate(const LT::date& valueDate, const CalibrationInstruments& instruments)
    {
        LT::date futuresStartDate(valueDate);

        // initialize last futures end date ( could be stored in a member variable for reuse in createKnotPoints )
        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
            if(isOfType<Futures>(*iter) && ((*iter)->getStartDate() <= futuresStartDate || futuresStartDate == valueDate) )
            {
                futuresStartDate = (*iter)->getStartDate();
            }
        }
        return futuresStartDate;
    }

    LT::date getFirstFRAStartDate(const LT::date& valueDate, const CalibrationInstruments& instruments)
    {
        LT::date fraStartDate(valueDate);

        // initialize last futures end date ( could be stored in a member variable for reuse in createKnotPoints )
        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
            if(isOfType<ForwardRateAgreement>(*iter) && ((*iter)->getStartDate() <= fraStartDate || fraStartDate == valueDate) )
            {
                fraStartDate = (*iter)->getStartDate();
            }
        }
        return fraStartDate;
    }

    LT::date getLastFRAEndDate(const LT::date& valueDate, const CalibrationInstruments& instruments)
    {
        LT::date lastFRAEndDate(valueDate);

        // initialize last futures end date ( could be stored in a member variable for reuse in createKnotPoints )
        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
            if(isOfType<ForwardRateAgreement>(*iter) && (*iter)->getEndDate() > lastFRAEndDate)
            {
                lastFRAEndDate = (*iter)->getEndDate();
            }
        }
        return lastFRAEndDate;
    }
     
	LT::date getLastOISEndDate(const LT::date& valueDate, const CalibrationInstruments& instruments)
    {
        LT::date lastOISEndDate(valueDate);

        // initialize last futures end date ( could be stored in a member variable for reuse in createKnotPoints )
        for(CalibrationInstruments::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
        {
            if(isOfType<OvernightIndexedSwap>(*iter) && (*iter)->getEndDate() > lastOISEndDate)
            {
                lastOISEndDate = (*iter)->getEndDate();
            }
        }
        return lastOISEndDate;
    }
    
	bool getFirstCashEndDateOnOrAfterGivenDate(const LT::date& date, const CalibrationInstruments& instruments, LT::date& cashEndDate)
    {
        typedef std::vector<CalibrationInstrumentPtr> CashInstruments;
		
		//	Extract all cash instruments
		CashInstruments cashInstruments;
		instruments.extractInstrumentsOfType<CashInstrument>(cashInstruments);
		
		for(CashInstruments::const_iterator iter(cashInstruments.begin()); iter != cashInstruments.end(); ++iter)
		{
			if( (*iter)->getEndDate() >= date )
			{
                cashEndDate = (*iter)->getEndDate();
				return true;
			}
		}
		return false;
    }
	
    bool cashInstrumentOfGivenTenorExists(const CalibrationInstruments& instruments,
										  const CurveTypeConstPtr& tenor)
	{
		typedef std::vector<CalibrationInstrumentPtr> CashInstruments;
		
		//	Extract all cash instruments
		CashInstruments cashInstruments;
		instruments.extractInstrumentsOfType<CashInstrument>(cashInstruments);
		
		for(CashInstruments::const_iterator iter(cashInstruments.begin());
			iter != cashInstruments.end(); ++iter)
		{
			if(CurveType::getFromDescription((*iter)->getDescription()) == tenor)
			{
				return true;
			}
		}
		return false;
	}

	void selectInstrumentsForStripperModel(const BaseModel& model, CalibrationInstruments& instruments, CashSelection cashSelection)
	{
		const StripperModel& stripperModel(dynamic_cast<const StripperModel&>(model));
		const CurveTypeConstPtr modelTenor(CurveType::getFromYearFraction(stripperModel.getTenor()));
		const LT::date lastFuturesEndDate(getLastFuturesEndDate(model.getValueDate(), instruments));

		removeNonCashFuturesFRASwaps(instruments);
        // Remove FRAs if there are non-expired futures
        const bool selectFutures = lastFuturesEndDate > model.getValueDate();
        if( selectFutures )
        {
            removeFRAs( instruments );
        }
        else
        {
            removeNonBaseRateFRAs(modelTenor, instruments);
        }
        const LT::date lastFRAEndDate(getLastFRAEndDate(model.getValueDate(), instruments));
		removeFixedFutures(model.getValueDate(), instruments);

        if( selectFutures || ( lastFRAEndDate >  model.getValueDate() ) )
        {
			selectCashInstrumentsForStripperModel(model, instruments, cashSelection);
			erase_if(instruments, [selectFutures, &lastFuturesEndDate, &lastFRAEndDate] (const CalibrationInstrumentPtr& instrument) 
			  {return swapAndLessThanFuturesEndDate(instrument, selectFutures ? lastFuturesEndDate : lastFRAEndDate);});
        }
    }


    void selectCashInstrumentsForStripperModel(const BaseModel& model, CalibrationInstruments& instruments, CashSelection cashSelection)
	{
		if( CashSelection::BaseRate == cashSelection )
        {
            const StripperModel& stripperModel(dynamic_cast<const StripperModel&>(model));
		    const CurveTypeConstPtr modelTenor(CurveType::getFromYearFraction(stripperModel.getTenor()));
            removeNonBaseCashRates(modelTenor, instruments);
        }
        else if ( CashSelection::FutureStartLinear == cashSelection )
        {

            const LT::date startDate( max( getFirstFuturesStartDate(model.getValueDate(), instruments), getFirstFRAStartDate(model.getValueDate(), instruments) ) );
            LT::date cashEndDate;

            bool hasCashAfterFirstFuture = getFirstCashEndDateOnOrAfterGivenDate(startDate, instruments, cashEndDate);
            if( hasCashAfterFirstFuture )
            {
                removeCashRatesAfterGivenDate(cashEndDate, instruments);
                std::vector<CalibrationInstrumentPtr> cashInstruments;
		        instruments.extractInstrumentsOfType<CashInstrument>(cashInstruments);
                if( (*cashInstruments.rbegin())->getEndDate() > startDate)
                {
                    double interpolatedRate = interpolatedZeroRateFromCash( model, instruments, startDate);
		            CashInstrumentPtr tmpCashInstrument = std::tr1::dynamic_pointer_cast<CashInstrument>(*cashInstruments.rbegin());
                    CalibrationInstrumentPtr cash(new CashInstrument(tmpCashInstrument->getDescription().data(), 
                                                                 model.getValueDate(), 
                                                                 tmpCashInstrument->getStartDate(), 
                                                                 startDate, 
															     interpolatedRate, 
															     tmpCashInstrument->basis(),
															     model.getValueDate()));

                    instruments.add(cash);
                    removeCashRatesAfterGivenDate(startDate, instruments);
                }
            }
        }
        else if (CashSelection::FutureStartBlending == cashSelection )
        {
            LT::date futureStartDate( max( getFirstFuturesStartDate(model.getValueDate(), instruments), getFirstFRAStartDate(model.getValueDate(), instruments) ) ), cashEndDate;
            bool hasCashAfterFirstFuture = getFirstCashEndDateOnOrAfterGivenDate(futureStartDate, instruments, cashEndDate);
            if( hasCashAfterFirstFuture )
            {
                removeCashRatesAfterGivenDate(cashEndDate, instruments);
            }
        }
        else
        {
            LT_THROW_ERROR( "Could not recognise cash selection rule " << cashSelection.asString().data() );
        }
	}

    double interpolatedZeroRateFromCash(const BaseModel& model, const CalibrationInstruments& instruments, const LT::date& inputDay)
    {
        typedef std::vector<CalibrationInstrumentPtr> CashInstruments;
		CashInstruments cashInstruments;
		instruments.extractInstrumentsOfType<CashInstrument>(cashInstruments);
		
        const LT::date valueDate = model.getValueDate();
        std::map<LT::date, std::vector<CalibrationInstrumentPtr> > cashMap;
        std::map<LT::date, std::pair<LT::date, double> > spotDateAccrualMap;
		for(CashInstruments::const_iterator iter(cashInstruments.begin());iter != cashInstruments.end(); ++iter)
		{
            cashMap[(*iter)->getStartDate()].push_back(*iter);
		    CashInstrumentPtr tmpCashInstrument = std::tr1::dynamic_pointer_cast<CashInstrument>(*iter);
            spotDateAccrualMap[(*iter)->getEndDate()] = std::pair<LT::date, double>((*iter)->getStartDate(), tmpCashInstrument->getCoverage());
		}

        std::map<LT::date, std::vector<std::pair<LT::date,double> > > cashDfs;
        std::map<LT::date, std::vector<CalibrationInstrumentPtr> >::const_iterator itCash = cashMap.begin(), itCashEnd = cashMap.end();
        for( ;itCash != itCashEnd; ++itCash)
        {
            std::vector<FlexYCF::CalibrationInstrumentPtr> cash = itCash->second;
            for(size_t i = 0; i < cash.size(); ++i)
            {

				CashInstrumentPtr tmpCashInstrument = std::tr1::dynamic_pointer_cast<CashInstrument>(cash[i]);

                double accrual = tmpCashInstrument->getCoverage();
                double df = 1.0/(1.0 + cash[i]->getRate() * accrual);
                cashDfs[itCash->first].push_back(std::pair<LT::date,double>(cash[i]->getEndDate(), df) );
            }
        }

        std::map<LT::date,double> dateDf;
        dateDf[valueDate] = 1.0;

        std::map<LT::date, std::vector<std::pair<LT::date,double> > >::const_iterator it = cashDfs.begin(), itEnd = cashDfs.end();
        for( ; it != itEnd ; ++it )
        {
            LT::date t = it->first;
            if( t ==  valueDate )
            {
                std::vector<std::pair<LT::date,double> > rates = it->second;
                for(size_t i = 0; i < rates.size(); ++i)
                {
                    dateDf[rates[i].first] = rates[i].second;
                }
            }
            else
            {
                double dfAdj = 1.0;
                std::map<LT::date,double>::const_iterator itDay = dateDf.find(t);
                if( itDay == dateDf.end() )
                {
                    LT::date lastKnowTime = dateDf.rbegin()->first;
                    double lastKnownDf = dateDf.rbegin()->second;
                    
                    double t1 = ModuleDate::getYearsBetween(valueDate, t );
                    double t0 = ModuleDate::getYearsBetween(valueDate, lastKnowTime );
                    double t2 = ModuleDate::getYearsBetween(valueDate, it->second[0].first );

                    dateDf[t] = lastKnownDf * ::pow(it->second[0].second, (t1 - t0)/(t2 - t1));
                    dfAdj = dateDf[t];
                }
                else
                {
                    dfAdj = itDay->second;
                }
                std::vector< std::pair<LT::date,double> > rates = it->second;
                for(size_t i = 0; i < rates.size(); ++i)
                {
                    dateDf[rates[i].first] = rates[i].second * dfAdj;
                }
            }
        }

        LT::date left, right;
        std::map<LT::date,double>::const_iterator itDfs = dateDf.lower_bound(inputDay);
        
        // extrapolation
        if( itDfs == dateDf.end() )
        {
            return (*cashInstruments.rbegin())->getRate();
        }
        if( itDfs == dateDf.begin() )
        {
            return (*cashInstruments.begin())->getRate();
        }

        // interpolation
        right = itDfs->first;
        --itDfs;
        left = itDfs->first;

        double t1 = ModuleDate::getYearsBetween(valueDate, left );
        double t2 = ModuleDate::getYearsBetween(valueDate, right );
        double t  = ModuleDate::getYearsBetween(valueDate, inputDay );
        double z1 = - ::log(dateDf[left]);
        double z2 = - ::log(dateDf[right]);

        double spot = ModuleDate::getYearsBetween(valueDate, spotDateAccrualMap[right].first);
		double interpolatedZero = ((t2 -t ) * z1 + (t - t1) * z2)/(t2 - t1);
        double tau = spotDateAccrualMap[right].second * (t - spot)/(t2 - spot);
        return (dateDf[spotDateAccrualMap[right].first]/exp( - interpolatedZero) - 1.0)/tau;
    }

	void removeDuplicateCashInstruments(CalibrationInstruments& instruments)
	{
		typedef std::vector<CalibrationInstrumentPtr> CashInstruments;
		
		CashInstruments cashInstruments;
		instruments.extractInstrumentsOfType<CashInstrument>(cashInstruments);
		auto it = cashInstruments.begin();
		std::set<LT::date> cashEndDates;
		for( ;it != cashInstruments.end(); ++it)
		{
			const LT::date& endDate = (*it)->getEndDate();
			if(cashEndDates.find(endDate) == cashEndDates.end())
			{
				cashEndDates.insert(endDate);
			}
			else
			{
				LT::Str desc = (*it)->getDescription();
				erase_if(instruments, [&desc] (const CalibrationInstrumentPtr& instrument) {return isCashAndHasTenor(instrument, desc);});
			}
		}
	}

	void removeDuplicateCrossCurrencySwaps(CalibrationInstruments& instruments)
	{
		typedef std::vector<CalibrationInstrumentPtr> SwapInstruments;
		
		SwapInstruments swapInstruments;
		instruments.extractInstrumentsOfType<CrossCurrencySwap>(swapInstruments);
		auto it = swapInstruments.begin();
		std::set<LT::date> swapEndDates;
		for( ;it != swapInstruments.end(); ++it)
		{
			const LT::date& endDate = (*it)->getEndDate();
			if(swapEndDates.find(endDate) == swapEndDates.end())
			{
				swapEndDates.insert(endDate);
			}
			else
			{
				LT::Str desc = (*it)->getDescription();
				erase_if(instruments, [&desc] (const CalibrationInstrumentPtr& instrument) {return isCrossCurrencySwapAndHasTenor(instrument, desc);});
			}
		}
	}

	void removeDuplicateTenorBasisSwaps(CalibrationInstruments& instruments, size_t numberOfDatesToTest)
	{
		typedef std::vector<CalibrationInstrumentPtr> SwapInstruments;
		
		SwapInstruments swapInstruments;
		instruments.extractInstrumentsOfType<TenorBasisSwap>(swapInstruments);
		auto it = swapInstruments.begin();
		std::string otherTenor;
		if(it != swapInstruments.end())
		{
			const TenorBasisSwapPtr tenorBasisSwap(std::tr1::dynamic_pointer_cast<TenorBasisSwap>((*it)));
			otherTenor = tenorBasisSwap->getOtherTenor();
		}
		std::set<LT::date> swapEndDates;
		for( ;it != swapInstruments.end() && swapEndDates.size() < numberOfDatesToTest; ++it)
		{
			const TenorBasisSwapPtr tenorBasisSwap(std::tr1::dynamic_pointer_cast<TenorBasisSwap>((*it)));
			const std::string tenor = tenorBasisSwap->getOtherTenor();
			const LT::date& endDate = (*it)->getEndDate();
			if(tenor.compare(otherTenor) == 0)
			{
				if(swapEndDates.find(endDate) == swapEndDates.end())
				{
					swapEndDates.insert(endDate);
				}
				else
				{
					LT::Str desc = (*it)->getDescription();
					erase_if(instruments, [&desc] (const CalibrationInstrumentPtr& instrument) {return isTenorBasisSwapAndHasTenor(instrument, desc);});
				}
			}
		}
	}
}