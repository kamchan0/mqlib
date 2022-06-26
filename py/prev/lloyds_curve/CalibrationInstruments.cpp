/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "CalibrationInstruments.h"
#include "CalibrationInstrument.h"
#include "BaseModel.h"
#include "Gradient.h"
#include "Data/GenericData.h"
#include "FlexYcfUtils.h"

#include "CashInstrument.h"
#include "ZeroRate.h"
#include "ZeroSpread.h"
#include "TenorZeroRate.h"
#include "ForwardZeroRate.h"
#include "CommodityFutures.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "TenorBasisSwap.h"
#include "CrossCurrencySwap.h"
#include "CrossCurrencyOISSwap.h"
#include "CrossCurrencyFundingSwap.h"
#include "OvernightIndexedSwap.h"
#include "OISBasisSwap.h"
#include "FxForward.h"
//	#include "ZCILInflationSwap.h"

#include "DataExtraction.h"
#include "DictYieldCurve.h"
#include "FlexYCFCloneLookup.h"

using namespace std;

using namespace LTQC;

namespace 
{
	void fillTenor(const LTQuant::GenericDataPtr& instrumentListData,
				   const FlexYCF::CalibrationInstrument& instrument,
				   const size_t index)
	{
		instrumentListData->set<std::string>("Tenor", index, instrument.getDescription().string());
	}

	void fillDates(const LTQuant::GenericDataPtr& instrumentListData, 
				   const FlexYCF::CalibrationInstrument& instrument,
				   const size_t index)
	{
		instrumentListData->set<LT::date>("Start Date", index, instrument.getStartDate());
		instrumentListData->set<LT::date>("End Date", index, instrument.getEndDate());		
	}
}


namespace FlexYCF
{
    template<>
	const IDeA::DictionaryKey& getKey<CalibrationInstruments>()
	{
		return IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST);
	}

    void CalibrationInstruments::add(const CalibrationInstrumentPtr& calibrationInstrument)
    {
        m_instruments.push_back(calibrationInstrument);
    }

	double CalibrationInstruments::getResidual(const BaseModelPtr& model,
											   const size_t index) const
	{
		return m_instruments[index]->getResidual(model);
	}

	void CalibrationInstruments::computeResidualGradient(const BaseModelPtr& model,
														 const size_t index,
														 Gradient& gradient) const
	{
		// This is because an instrument residual is the 
		//	different between its model and market prices
		fillWithZeros(gradient, m_instruments.size());
		gradient[index] = m_instruments[index]->calculateRateDerivative(model);
	}

    double CalibrationInstruments::getResidualsNorm(const BaseModelPtr model) const
    {
        double norm(0.0);
        double residual;
        for(InstrumentContainer::const_iterator iter(m_instruments.begin()); iter != m_instruments.end(); ++iter)
        {
            residual = (*iter)->getResidual(model); 
            norm += (residual * residual);
        }
        return norm;
    }

	void CalibrationInstruments::extractInstrumentsOfType(const std::string& instrumentTypeName, 
														  std::vector<CalibrationInstrumentPtr>& instruments) const
	{
		instruments.clear();
		for(CalibrationInstruments::const_iterator iter(m_instruments.begin()); iter != m_instruments.end(); ++iter)
		{
			// The move constructor called from remove_if can cause *iter to be null
			if(*iter != nullptr && (*iter)->getType() == instrumentTypeName)
			{
				instruments.push_back(*iter);
			}
		}
	}

	LTQuant::GenericDataPtr CalibrationInstruments::asGenericData() const
	{
		LTQuant::GenericDataPtr instrumentsList(new LTQuant::GenericData("Instruments List", 0));
		
		tryToAddInstrumentTable<CashInstrument>(instrumentsList);
        tryToAddInstrumentTable<ZeroRate>(instrumentsList);
		tryToAddInstrumentTable<ZeroSpread>(instrumentsList);
		tryToAddInstrumentTable<TenorZeroRate>(instrumentsList);
		tryToAddInstrumentTable<ForwardZeroRate>(instrumentsList);
        tryToAddInstrumentTable<CommodityFutures>(instrumentsList);
		tryToAddInstrumentTable<Futures>(instrumentsList);
		tryToAddInstrumentTable<InterestRateSwap>(instrumentsList);
		tryToAddInstrumentTable<CrossCurrencySwap>(instrumentsList);
        tryToAddInstrumentTable<CrossCurrencyOISSwap>(instrumentsList);
        tryToAddInstrumentTable<CrossCurrencyFundingSwap>(instrumentsList);
		tryToAddInstrumentTable<TenorBasisSwap>(instrumentsList);
        tryToAddInstrumentTable<OvernightIndexedSwap>(instrumentsList);
		tryToAddInstrumentTable<OISBasisSwap>(instrumentsList);

        return instrumentsList;
	}

	//	Cash Instrument
	template<>
	void CalibrationInstruments::fillData<CashInstrument>(const LTQuant::GenericDataPtr& instrumentListData, 
														  const CashInstrument& instrument,
														  const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Rate", index, instrument.getRate());
	}
    
    //	Zero Rate
	template<>
	void CalibrationInstruments::fillData<ZeroRate>(const LTQuant::GenericDataPtr& instrumentListData, 
														  const ZeroRate& instrument,
														  const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Rate", index, instrument.getRate());
	}

	 //	Zero Spread
	template<>
	void CalibrationInstruments::fillData<ZeroSpread>(const LTQuant::GenericDataPtr& instrumentListData, 
														  const ZeroSpread& instrument,
														  const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Rate", index, instrument.getRate());
	}

	 //	Tenor Zero Rate
	template<>
	void CalibrationInstruments::fillData<TenorZeroRate>(const LTQuant::GenericDataPtr& instrumentListData, 
														  const TenorZeroRate& instrument,
														  const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Rate", index, instrument.getRate());
	}

	//	Forward Zero Rate
	template<>
	void CalibrationInstruments::fillData<ForwardZeroRate>(	const LTQuant::GenericDataPtr& instrumentListData, 
															const ForwardZeroRate& instrument,
															const size_t index)
	{
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Rate", index, instrument.getRate());
	}

    //	Commodity Futures
	template<>
	void CalibrationInstruments::fillData<CommodityFutures>(const LTQuant::GenericDataPtr& instrumentListData,
												   const CommodityFutures& instrument,
												   const size_t index)
	{
		instrumentListData->set<std::string>("Expiry", index, instrument.getDescription().string());
		
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Price", index, instrument.getRate());
		instrumentListData->set<double>("Convexity", index, instrument.getConvexityAdjustment());
	}
	
    //	Futures
	template<>
	void CalibrationInstruments::fillData<Futures>(const LTQuant::GenericDataPtr& instrumentListData,
												   const Futures& instrument,
												   const size_t index)
	{
		instrumentListData->set<std::string>("Expiry", index, instrument.getDescription().string());
		
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Price", index, 1.0 - instrument.getRate());
		instrumentListData->set<double>("Convexity", index, instrument.getConvexityAdjustment());
	}

	// Interest Rate Swap
	template<>
	void CalibrationInstruments::fillData<InterestRateSwap>(const LTQuant::GenericDataPtr& instrumentListData,
															const InterestRateSwap& instrument,
														    const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Rate", index, instrument.getRate());
		instrumentListData->set<std::string>("Fxd Tenor", index, instrument.getFixedLegTenorDesc());
		instrumentListData->set<std::string>("Flt Tenor", index, instrument.getFloatingLegTenorDesc());
		instrumentListData->set<std::string>("Fxd Basis", index, instrument.getFixedBasis());
		instrumentListData->set<std::string>("Flt Basis", index, instrument.getFloatBasis());
	}

	//	Cross-Currency Swaps
	template<>
	void CalibrationInstruments::fillData<CrossCurrencySwap>(const LTQuant::GenericDataPtr& instrumentListData,
															 const CrossCurrencySwap& instrument,
															 const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);

		instrumentListData->set<double>("Spread", index, instrument.getRate());
		instrumentListData->set<std::string>("Domestic Flt Tenor", index, instrument.getDomesticFloatingLegTenorDesc());
		instrumentListData->set<std::string>("Domestic Flt Basis", index, instrument.getDomesticFloatBasis());
		instrumentListData->set<std::string>("Foreign Flt Basis", index, instrument.getForeignFloatingLegTenorDesc());
		
		fillDates(instrumentListData, instrument, index);
	}
    
    //	Cross-Currency OIS Swaps
	template<>
	void CalibrationInstruments::fillData<CrossCurrencyOISSwap>(const LTQuant::GenericDataPtr& instrumentListData,
															 const CrossCurrencyOISSwap& instrument,
															 const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);

		instrumentListData->set<double>("Spread", index, instrument.getRate());
		
		fillDates(instrumentListData, instrument, index);
	}
    
    //	Cross-Currency Funding Swaps
	template<>
	void CalibrationInstruments::fillData<CrossCurrencyFundingSwap>(const LTQuant::GenericDataPtr& instrumentListData,
															 const CrossCurrencyFundingSwap& instrument,
															 const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);

		instrumentListData->set<double>("Spread", index, instrument.getRate());
		
		fillDates(instrumentListData, instrument, index);
	}

	//	Tenor Basis Swaps
	template<>
	void CalibrationInstruments::fillData<TenorBasisSwap>(const LTQuant::GenericDataPtr& instrumentListData,
														  const TenorBasisSwap& instrument,
														  const size_t index)
	{
		instrumentListData->set<std::string>("Maturity", index, instrument.getDescription().string());
		instrumentListData->set<std::string>("Tenor", index, instrument.getOtherTenor());

		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Spread", index, instrument.getRate());
		
		IDeA::inject<std::string>(*instrumentListData, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, TENOR1), index, instrument.getLeg1TenorDesc());
		IDeA::inject<std::string>(*instrumentListData, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ACCRUALBASIS1), index, instrument.getLeg1AccrualBasis());
		IDeA::inject<std::string>(*instrumentListData, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, TENOR2), index, instrument.getLeg2TenorDesc());
		IDeA::inject<std::string>(*instrumentListData, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ACCRUALBASIS2), index, instrument.getLeg2AccrualBasis());
		IDeA::inject<std::string>(*instrumentListData, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPREADTENOR), index, instrument.getSpreadLegTenorDesc());
		IDeA::inject<std::string>(*instrumentListData, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, SPREADACCRUALBASIS), index, instrument.getSpreadLegAccrualBasis());
	}

    // OIS
    template<>
	void CalibrationInstruments::fillData<OvernightIndexedSwap>(const LTQuant::GenericDataPtr& instrumentListData,
															const OvernightIndexedSwap& instrument,
														    const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Rate", index, instrument.getRate());
		instrumentListData->set<std::string>("Fxd Tenor", index, instrument.getFixedLegTenorDesc());
		instrumentListData->set<std::string>("Flt Tenor", index, instrument.getFloatingLegTenorDesc());
		instrumentListData->set<std::string>("Fxd Basis", index, instrument.getFixedBasis());
		instrumentListData->set<std::string>("Flt Basis", index, instrument.getFloatBasis());
        instrumentListData->set<std::string>("Rates Basis", index, instrument.getRatesBasis());
        instrumentListData->set<std::string>("Spot Days", index, instrument.getSpotDays());
		instrumentListData->set<std::string>("Pay Delay", index, instrument.getPayDelay());
        instrumentListData->set<std::string>("Pay Calendar", index, instrument.getPayCalendar());
        instrumentListData->set<std::string>("Index", index, instrument.getIndex());
	}
 
    // OISBASIS
    template<>
	void CalibrationInstruments::fillData<OISBasisSwap>(const LTQuant::GenericDataPtr& instrumentListData,
															const OISBasisSwap& instrument,
														    const size_t index)
	{
		fillTenor(instrumentListData, instrument, index);
		fillDates(instrumentListData, instrument, index);

		instrumentListData->set<double>("Rate", index, instrument.getRate());
		instrumentListData->set<std::string>("Spread Tenor", index, instrument.getSpreadLegTenorDesc());
		instrumentListData->set<std::string>("Flt Tenor", index, instrument.getFloatingLegTenorDesc());
		instrumentListData->set<std::string>("Spread Basis", index, instrument.getFixedBasis());
		instrumentListData->set<std::string>("Flt Basis", index, instrument.getFloatBasis());
        instrumentListData->set<std::string>("Rates Basis", index, instrument.getRatesBasis());
        instrumentListData->set<std::string>("Spot Days", index, instrument.getSpotDays());
		instrumentListData->set<std::string>("Pay Delay", index, instrument.getPayDelay());
        instrumentListData->set<std::string>("Pay Calendar", index, instrument.getPayCalendar());
        instrumentListData->set<std::string>("Index", index, instrument.getIndex());
	}
	
    /*
	template<>
	void CalibrationInstruments::iterateAndFillData<TenorBasisSwap>(const LTQuant::GenericDataPtr& instrumentListData,
																	const InstrumentContainer& instruments)
	{
		using namespace LTQuant;
		
		const static string maturityTag("Maturity");
		const static string tenorTag("Tenor");
		const static string valuesTag("Values");

		GenericDataPtr maturityData(new GenericData(maturityTag, 0));
		GenericDataPtr tenorData(new GenericData(tenorTag, 0));
		GenericDataPtr valuesData(new GenericData(valuesTag, 0));

		instrumentListData->set<GenericDataPtr>(maturityTag, 0, maturityData);
		instrumentListData->set<GenericDataPtr>(tenorTag, 0, tenorData);
		instrumentListData->set<GenericDataPtr>(valuesTag, 0, valuesData);

		//	First pass: remember maturities and tenors
		std::vector<std::string> maturities, tenors;

		std::string tenor;
		for(InstrumentContainer::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
		{
			if(std::find(maturities.begin(), maturities.end(), (*iter)->getDescription()) == maturities.end())
			{		
				maturities.push_back((*iter)->getDescription());
			}

			tenor = dynamic_cast<const TenorBasisSwap&>(**iter).getTenor();
			if(std::find(tenors.begin(), tenors.end(), tenor) == tenors.end())
			{
				tenors.push_back(tenor);
			}
		}
		
		//	Sort them
		//sort(maturities.begin(), maturities.end());
		//sort(tenors.begin(), tenors.end());
		
		//	Fill Maturity data
		for(size_t mat(0); mat < maturities.size(); ++mat)
		{
			maturityData->set<std::string>(maturityTag, index, maturities[mat]);	
		}

		//	Fill Tenor data
		for(size_t t(0); t < tenors.size(); ++t)
		{
			tenorData->set<std::string>(tenorTag, index, tenors[t]);
		}
		
		//	Fill Values data
		for(InstrumentContainer::const_iterator iter(instruments.begin()); iter != instruments.end(); ++iter)
		{
			valuesData->getValueItem(
				std::find(tenors.begin(), tenors.end(), dynamic_cast<const TenorBasisSwap&>(**iter).getTenor()) - tenors.begin(),
				std::find(maturities.begin(), maturities.end(), (*iter)->getDescription()) - maturities.begin()
									) = (*iter)->getRate();
		}	
	}
	*/
    ostream& CalibrationInstruments::print(ostream& out) const
    {
        for(InstrumentContainer::const_iterator iter(m_instruments.begin()); iter != m_instruments.end(); ++iter)
        {
            out << (*iter) << endl;
        }
        return out;
    }

    /**
        @brief Clone these calibration instruments.

        The clone is performed using a lookup table to ensure that directed graph relationships are maintained.

        @param lookup The lookup of already cloned instances.
        @return       The clone.
    */
    ICloneLookupPtr CalibrationInstruments::cloneWithLookup(CloneLookup& lookup) const
    {
        CalibrationInstrumentsPtr clone(new CalibrationInstruments());
        clone->assign(*this, lookup);
        return clone;
    }

    /**
        @brief Assign and clone instruments.

        Assign the clones of instruments in another collection to this collection (overwriting existing). The cloning
        is done using a lookup to ensure directed graph relationships are maintained.

        @param from   Assign from this instance.
        @param lookup The lookup of previously created instances.
    */
    void CalibrationInstruments::assign(CalibrationInstruments const& from, CloneLookup& lookup)
    {
        CloneLookupUtils::assign(from.m_instruments, m_instruments, lookup);
    }

}   //  FlexYCF