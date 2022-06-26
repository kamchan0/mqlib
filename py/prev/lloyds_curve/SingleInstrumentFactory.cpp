/*****************************************************************************
    
	SingleInstrumentFactory


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "SingleInstrumentFactory.h"
#include "CashInstrument.h"
#include "ForwardRateAgreement.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "CrossCurrencySwap.h"
#include "CrossCurrencyOISSwap.h"
#include "CrossCurrencyFundingSwap.h"
#include "TenorBasisSwap.h"
#include "OvernightIndexedSwap.h"
#include "OISBasisSwap.h"
#include "FixedRateBond.h"
#include "InflationBond.h"

using namespace LTQC;

namespace FlexYCF
{

	CalibrationInstrumentPtr SingleInstrumentFactory::createInstrument(const std::string& instrumentTypeName,
																	   const LTQuant::GenericData& instrumentParametersTable,
																	   const LT::Ptr<LT::date>& buildDate,
																	   const LTQuant::GenericData& curveParametersTable,
																	   const LTQuant::GenericDataPtr& extraInfoTable)
	{
		return SingleInstrumentFactory::create(instrumentTypeName)(instrumentParametersTable, buildDate, curveParametersTable, extraInfoTable);
	}

	SingleInstrumentFactory::SingleInstrumentFactory()
	{
		registerInstrument<CashInstrument>();
        registerInstrument<ForwardRateAgreement>();
		registerInstrument<Futures>();
		registerInstrument<InterestRateSwap>();
		registerInstrument<CrossCurrencySwap>();
        registerInstrument<CrossCurrencyOISSwap>();
         registerInstrument<CrossCurrencyFundingSwap>();
		registerInstrument<TenorBasisSwap>();
		registerInstrument<OvernightIndexedSwap>();
        registerInstrument<OISBasisSwap>();
		registerInstrument<FixedRateBondInstrument>();
		registerInstrument<InflationBondInstrument>();
	}

	
    SingleInstrumentFactory* const SingleInstrumentFactory::instance()
    {
        static SingleInstrumentFactory theSingleInstrumentFactory;
        return &theSingleInstrumentFactory;
    }
}