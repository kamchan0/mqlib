/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "ILZCSwapInstrument.h"
#include "Data\GenericData.h"
#include "Data\MarketData\YieldCurveCreator.h"
#include "BaseModel.h"
#include "ModuleDate/InternalInterface/CalendarFactory.h"
#include "InflationModel.h"
#include "GlobalComponentCache.h"
#include "FlexYcfUtils.h"
#include "ScheduleUtils.h"

//	IDeA
#include "DictMarketData.h"
#include "DictionaryManager.h"
#include "DataExtraction.h"


using namespace std;

using namespace LTQC;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<ILZCSwapInstrument>()
	{
		return IDeA_KEY(INSTRUMENTLIST, ILZCSWAP);
	}


    using namespace LTQuant;
    using namespace ModuleDate;

    namespace 
    {
        // lookup the entry in the asset table corresponding to the
        // asset name, fail if not found
        size_t findAssetEntry(const string& assetName, GenericDataPtr assetTable)
        {
            for(size_t i(0); i < assetTable->numItems() - 1; ++i)
            {
                if(compareNoCase(assetTable->get<string>("Name", i), assetName))
                {
                    return i;
                }
            }
            LT_THROW_ERROR("Can't find asset " << assetName);
        }

        double frequencyToPower(const string& freqString)
        {
            if(compareNoCase(freqString, "Annual"))
            {
                return 1.0;
            }
            else if(compareNoCase(freqString, "A"))
            {
                return 1.0;
            }
            else if(compareNoCase(freqString, "SemiAnnual"))
            {
                return 2.0;
            }
            else if(compareNoCase(freqString, "Semi"))
            {
                return 2.0;
            }
            else if(compareNoCase(freqString, "S"))
            {
                return 2.0;
            }
            else if(compareNoCase(freqString, "Quarterly"))
            {
                return 4.0;
            }
            else if(compareNoCase(freqString, "Q"))
            {
                return 4.0;
            }
            else
            {
                LT_THROW_ERROR("Invalid Frequency String: " << freqString);
            }
        }

    }

    // construct an Inflation Zero Coupon Swap instrument from the table data
    ILZCSwapInstrument::ILZCSwapInstrument(LT::date buildDate, 
                                           const string& maturity, 
                                           const string& mktConventionName, 
                                           double yield, 
                                           GenericDataPtr mktConventionTable, 
                                           GenericDataPtr assetTable, 
                                           size_t mktConventionEntry,
										   GlobalComponentCache& globalComponentCache) :
		CalibrationInstrument(yield, mktConventionName, maturity, LT::date(), LT::date(), LT::date()),
        m_marketYield(yield)
    {
        const GenericDataPtr thisMktConventionTable(mktConventionTable->doesTagExist("Market Convention") ? 
													mktConventionTable->get<GenericDataPtr>("Market Convention", mktConventionEntry) : 
													mktConventionTable->get<GenericDataPtr>("MarketConvention", mktConventionEntry));
        const string spotDays(thisMktConventionTable->doesTagExist("Spot Days") ?
							  thisMktConventionTable->get<string>("Spot Days", 0) :
							  thisMktConventionTable->get<string>("SpotDays", 0));
        const string settleCalName(thisMktConventionTable->doesTagExist("Settle Calendar") ?
								   thisMktConventionTable->get<string>("Settle Calendar", 0) :
								   thisMktConventionTable->get<string>("SettleCalendar", 0));
        const string freqName(thisMktConventionTable->get<string>("Frequency", 0));

        const string assetName(thisMktConventionTable->get<string>("Asset", 0));

		const size_t assetEntry(findAssetEntry(assetName, assetTable));
        const GenericDataPtr thisAsset(assetTable->get<GenericDataPtr>("Asset", assetEntry));
        string timeLag(thisAsset->doesTagExist("IL Reset Time Lag") ?
					   thisAsset->get<string>("IL Reset Time Lag", 0) : 
					   thisAsset->get<string>("ILResetTimeLag", 0));
        const string resetInterp(thisAsset->doesTagExist("IL Reset Interp Type") ?
								 thisAsset->get<string>("IL Reset Interp Type", 0) :
								 thisAsset->get<string>("ILResetInterpType", 0));

		const CalendarPtr settleCal(CalendarFactory::create(settleCalName.c_str()));

// 		const string payCalName(thisMktConventionTable->doesTagExist("Pay Calendar") ?
// 								thisMktConventionTable->get<string>("Pay Calendar", 0) :
// 								thisMktConventionTable->get<string>("PayCalendar", 0));
// 		const CalendarPtr payCal(CalendarFactory::create(payCalName.c_str()));

        if(timeLag[0] != '-')
        {
            timeLag = "-" + timeLag;
        }

		const LT::date accrualStartDate(addDatePeriod(buildDate, spotDays, settleCal));
        const LT::date accrualEndDate(addDatePeriod(accrualStartDate, maturity, settleCal)); 
		setStartDate(accrualStartDate);
		setEndDate(accrualEndDate);

		const double years(getMonthsBetween(accrualStartDate, accrualEndDate) / 12.0);
		const double freqPower(frequencyToPower(freqName));
		m_tau = min(years, 1.0 / freqPower);
		m_timeUnits = years * freqPower;

		const ILIndexArg ilArgStart	= getInflationIndexArguments(buildDate, accrualStartDate, timeLag, resetInterp);
		const ILIndexArg ilArgEnd	= getInflationIndexArguments(buildDate, accrualEndDate,	  timeLag, resetInterp);

		// create the start and end inflation index instrument components
		m_indexStart =
			(InstrumentComponent::getUseCacheFlag()
								? globalComponentCache.get(InflationIndex::Arguments(ilArgStart.forward1Time, ilArgStart.forward2Time, ilArgStart.weight, accrualStartDate))
								: InflationIndex::create(InflationIndexArguments(ilArgStart.forward1Time, ilArgStart.forward2Time, ilArgStart.weight, accrualStartDate)));
		m_indexEnd =
			(InstrumentComponent::getUseCacheFlag()
								? globalComponentCache.get(InflationIndex::Arguments(ilArgEnd.forward1Time, ilArgEnd.forward2Time, ilArgEnd.weight, accrualEndDate))
								: InflationIndex::create(InflationIndexArguments(ilArgEnd.forward1Time, ilArgEnd.forward2Time, ilArgEnd.weight, accrualEndDate)));
	}

    // loop through the list of instruments and add an object for each entry.
    void ILZCSwapInstrument::createInstruments(CalibrationInstruments& instruments, 
                                               GenericDataPtr instrumentTable,
                                               GenericDataPtr data,
                                               GlobalComponentCache& globalComponentCache,
                                               const LTQuant::PriceSupplierPtr)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

        const GenericDataPtr detailsTable(data->get<GenericDataPtr>("Curve Details", 0));
        const LT::date buildDate(detailsTable->get<LT::date>("Build Date", 0));
        const GenericDataPtr staticTable(data->doesTagExist("Static Data") ? 
										 data->get<GenericDataPtr>("Static Data", 0) : 
										 data->get<GenericDataPtr>("StaticData", 0));
        const GenericDataPtr mktConventionTable(staticTable->doesTagExist("Market Conventions") ? 
											    staticTable->get<GenericDataPtr>("Market Conventions", 0) : 
												staticTable->get<GenericDataPtr>("MarketConventions", 0));
        const GenericDataPtr assetTable(staticTable->get<GenericDataPtr>("Assets", 0));
        for(size_t i(0); i < instrumentTable->numItems() - 1; ++i)
        {
            const string mktConventionName(instrumentTable->doesTagExist("Market Convention") ? 
										   instrumentTable->get<string>("Market Convention", i) : 
										   instrumentTable->get<string>("MarketConvention", i));
            if(!mktConventionName.empty())
            {
                const double yield(instrumentTable->get<double>("Value", i));
				const string maturity(instrumentTable->get<string>("Maturity", i));
                const size_t mktConventionEntry(findAssetEntry(mktConventionName, mktConventionTable));
                const CalibrationInstrumentPtr instrument(new ILZCSwapInstrument(buildDate, maturity, mktConventionName, yield, mktConventionTable,  assetTable, mktConventionEntry, globalComponentCache));
                instruments.add(instrument);
            }
        }
    }
    
    void ILZCSwapInstrument::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
    }
    void ILZCSwapInstrument::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
        CalibrationInstrument::reloadInternalState(src);
    }
    void ILZCSwapInstrument::updateInstruments(CalibrationInstruments& instrumentList, 
                                              LTQuant::GenericDataPtr instrumentTable, 
                                              size_t* instrumentIndex)
    {
		const size_t nbZCInflSwaps(IDeA::numberOfRecords(*instrumentTable));
        
		// just finish if we have empty table or just headings
        if(0 == nbZCInflSwaps)
        {
            return;
        }

        for(size_t i(0); i < nbZCInflSwaps; ++i)
        {
            const string description(instrumentTable->get<string>("Description", i));   // Date description (e.g. 6M)
            if(!description.empty())
            {
                const double yield(instrumentTable->get<double>("Value", i));     
                
                instrumentList[*instrumentIndex]->setRate(yield);
                ++(*instrumentIndex);
            }
        }
    }

    const double ILZCSwapInstrument::computeModelPrice(const BaseModelPtr model) const
    {
		const double index1(m_indexStart->getValue(*model));
		const double index2(m_indexEnd->getValue(*model));
		
        const double rate((pow(index2 / index1, min(1.0 / m_timeUnits, 1.0)) - 1.0) / m_tau);
        
		return rate;
    }

    double ILZCSwapInstrument::getLastRelevantTime() const
    {
		return m_indexEnd->getArguments().getForward2Time();
    }
    
	// Calculation of the gradient is directly based on the formula in computeModelPrice
    void ILZCSwapInstrument::accumulateGradient(BaseModel const& model,
												double /* multiplier */,
												GradientIterator gradientBegin,
												GradientIterator gradientEnd)
    {
		const double index1(m_indexStart->getValue(model));
		const double index2(m_indexEnd->getValue(model));
		const double exponent(min(1.0, 1.0 / m_timeUnits));
		const double coef(exponent * pow(index2 / index1, exponent - 1.) / (index1 * index1 * m_tau));

		m_indexEnd->accumulateGradient(model, coef * index1, gradientBegin, gradientEnd);
		m_indexStart->accumulateGradient(model, -coef * index2, gradientBegin, gradientEnd);
    }
	
	void ILZCSwapInstrument::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		USE(baseModel)
		USE(dfModel)
		USE(multiplier)
		USE(gradientBegin)
		USE(gradientEnd)
		USE(spread)
		LTQC_THROW(IDeA::SystemException, "accumulateGradientConstantDiscountFactor not implemented");
	}
	
	void ILZCSwapInstrument::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread)
	{
		USE(baseModel)
		USE(dfModel)
		USE(multiplier)
		USE(gradientBegin)
		USE(gradientEnd)
		USE(spread)
		LTQC_THROW(IDeA::SystemException, "accumulateGradientConstantTenorDiscountFactor not implemented");
	}
    
	void ILZCSwapInstrument::update()
    {
        m_indexStart->update();
		m_indexEnd->update();
    }

	double ILZCSwapInstrument::calculateRateDerivative(const BaseModelPtr& /* model */) const
	{
		return -1.0;
	}

	void ILZCSwapInstrument::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
		LT_THROW_ERROR( "Not implemented" );
	}

	void ILZCSwapInstrument::accumulateRateDerivativeGradient(const BaseModel& model,
												  double multiplier,
												  GradientIterator gradientBegin,
												  GradientIterator gradientEnd,
												  const CurveTypeConstPtr& curveType) const
	{
		LT_THROW_ERROR( "Not implemented" );
	}

	double ILZCSwapInstrument::computeParRate(const BaseModelPtr& /* model */)
	{
		LT_THROW_ERROR( "Not implemented" );
	}

	double ILZCSwapInstrument::getDifferenceWithNewRate(const LTQuant::GenericData& /* instrumentListData */) const
	{
		LT_THROW_ERROR( "Not implemented" );
	}

    /**
        @brief Clone this inflation zero-coupon swap.

        Implementation is trivial as this class has no other objects as member data.

        @return Returns a clone.
    */
    ICloneLookupPtr ILZCSwapInstrument::cloneWithLookup(CloneLookup& lookup) const
    {
		ILZCSwapInstrumentPtr clone(new ILZCSwapInstrument(*this));
		clone->m_indexStart = lookup.get(m_indexStart);
		clone->m_indexEnd	= lookup.get(m_indexEnd);
		return clone;
    }

	double ILZCSwapInstrument::getResetTimeLag() const
	{
		return m_indexStart->getArguments().getForward1Time();
	}

	double ILZCSwapInstrument::getForwardStartIndex(BaseModel const& inflationModel) const
	{
		return m_indexStart->getValue(inflationModel);
	}

	double ILZCSwapInstrument::getForwardEndIndex(BaseModel const& inflationModel) const
	{
		return m_indexEnd->getValue(inflationModel);
	}
	
	double ILZCSwapInstrument::getRateCoefficient() const
	{
		return pow(1.0 + m_tau * getMarketPrice(), max(1.0, m_timeUnits));
	}

}   // FlexYCF