/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "CalibrationInstrumentFactory.h"
#include "CalibrationInstruments.h"
#include "CashInstrument.h"
#include "ZeroRate.h"
#include "ZeroSpread.h"
#include "TenorZeroRate.h"
#include "ForwardZeroRate.h"
#include "FXForward.h"
#include "ForwardRateAgreement.h"
#include "CommodityFutures.h"
#include "Futures.h"
#include "CrossCurrencySwap.h"
#include "CrossCurrencyOISSwap.h"
#include "CrossCurrencyFundingSwap.h"
#include "InterestRateSwap.h"
#include "OvernightIndexedSwap.h"
#include "OISBasisSwap.h"
#include "TenorBasisSwap.h"
#include "FixedRateBond.h"
#include "InflationBond.h"
#include "Turn.h"
#include "Bump.h"
#include "Step.h"
#include "ILZCSwapInstrument.h"
#include "Data\GenericData.h"
#include "Pricers\PriceSupplier.h"
#include "Timer.h"
#include "FlexYcfUtils.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"



using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    using LTQuant::GenericDataPtr;

    CalibrationInstrumentFactory::CalibrationInstrumentFactory()
    {
        // we don't create instrument objects for the structural tables
		registerAndIgnore<Turn>();
		registerAndIgnore<Bump>();
		registerAndIgnore<Step>();
        
		registerObject("Seasonal", details::doNothingFunctions());
        registerInstrument<CashInstrument>();
        registerInstrument<ZeroRate>();
		registerInstrument<ZeroSpread>();
		registerInstrument<TenorZeroRate>();
		registerInstrument<ForwardZeroRate>();
		registerInstrument<FxForwardInstrument>();
        registerInstrument<ForwardRateAgreement>();
        registerInstrument<CommodityFutures>();
        registerInstrument<Futures>();
        registerInstrument<CrossCurrencySwap>();
        registerInstrument<CrossCurrencyOISSwap>();
        registerInstrument<CrossCurrencyFundingSwap>();
        registerInstrument<InterestRateSwap>();
		registerInstrument<OvernightIndexedSwap>();
        registerInstrument<OISBasisSwap>();
        registerInstrument<TenorBasisSwap>();
		registerInstrument<FixedRateBondInstrument>();
		registerInstrument<InflationBondInstrument>();
		// GDI-compatibility:
		//	Note: those should be supported with automatic IDeA aliases registration
		registerObject("Tenor Basis Swaps", 
						make_pair(*TenorBasisSwap::createInstruments, *TenorBasisSwap::updateInstruments));
		registerObject("Tenor Basis Swap",
						make_pair(*TenorBasisSwap::createInstruments, *TenorBasisSwap::updateInstruments));
        registerInstrument<ILZCSwapInstrument>();
    }

    void CalibrationInstrumentFactory::loadInstrumentList(CalibrationInstruments& instruments, 
                                                          GenericDataPtr data, 
                                                          GlobalComponentCache& globalComponentCache)
    {
        loadInstrumentList(instruments, data, globalComponentCache, LTQuant::PriceSupplierPtr());
    }

    void CalibrationInstrumentFactory::loadInstrumentList(CalibrationInstruments& instruments, 
                                                          GenericDataPtr data, 
                                                          GlobalComponentCache& globalComponentCache,
                                                          const LTQuant::PriceSupplierPtr priceSupplier)
    {
		const GenericDataPtr instrumentsTable(IDeA::extract<GenericDataPtr>(*data, getKey<CalibrationInstruments>()));
        for(size_t i(0); i < instrumentsTable->numTags(); ++ i)
        {
            string instrumentName(instrumentsTable->get<string>(i, 0));
            GenericDataPtr instrumentSubTable(instrumentsTable->get<GenericDataPtr>(i, 1));
         
            // CalibrationInstrumentCreationFunction creator = CalibrationInstrumentFactory::create(instrumentName);
            // creator
            CalibrationInstrumentFactory::create(instrumentName).first(instruments, 
                                                                       instrumentSubTable, 
                                                                       data, 
                                                                       globalComponentCache,
                                                                       priceSupplier);      
        }
    }
	
	void CalibrationInstrumentFactory::updateInstrumentRates(CalibrationInstruments& instruments, GenericDataPtr data)
    {
		const GenericDataPtr instrumentsTable(IDeA::extract<GenericDataPtr>(*data, getKey<CalibrationInstruments>()));
		FlexYCF::CalibrationInstruments::iterator it = instruments.begin();
        for(size_t i(0); i < instrumentsTable->numTags(); ++ i)
        {
            string instrumentName(instrumentsTable->get<string>(i, 0));
			LT::Str turns("Turns"), steps("Steps"), bumps("Bumps");
			
			if(instrumentName.find("Tenor") != string::npos && instrumentName.find("Basis") != string::npos)
			{
				GenericDataPtr instrumentSubTable(instrumentsTable->get<GenericDataPtr>(i, 1));
				if(!TenorBasisSwap::hasNewMarketDataFormat(instrumentSubTable))
				{	
					const GenericDataPtr tenorTable(IDeA::extract<GenericDataPtr>(*instrumentSubTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR)));
					const GenericDataPtr maturityTable(IDeA::extract<GenericDataPtr>(*instrumentSubTable, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY)));
					const GenericDataPtr valuesTable(IDeA::extract<GenericDataPtr>(*instrumentSubTable, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES)));
					const size_t nbTenors(IDeA::numberOfRecords(*tenorTable));
					const size_t nbMaturities(IDeA::numberOfRecords(*maturityTable));
				
					for(size_t cnt(0); cnt < nbMaturities; ++cnt)
					{
						for(size_t cnt2(0); cnt2 < tenorTable->numItems() - 1; ++cnt2)
						{
							const string tenorStr = IDeA::extract<std::string>(*tenorTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR), cnt2);

							if( !tenorStr.empty( ) ) // skip empty rows
							{
								double spread(valuesTable->get<double>(cnt2, cnt));
								(*it)->setRate(spread);
								++it;
							}
						}
					}
				}
				else
				{
					LT::TablePtr table = instrumentSubTable->table;
					size_t rows = table->rowsGet();
					size_t k1 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
					size_t k2 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());
			
					for(size_t row = 1; row < rows; ++row)
					{
						LT::Str tenorStr = table->at(row,k1);
						if(!tenorStr.empty())
						{
							LT::TablePtr basisSwapTable = table->at(row,k2);
							size_t k3 = basisSwapTable->findColKey(IDeA_KEY(BASISSWAP,RATE).getName());
							size_t k4 = basisSwapTable->findColKey(LT::NoThrow, IDeA_KEY(BASISSWAP,TENOR).getName());
							if(k4 == LT::Table::not_found)
							{
								k4 = basisSwapTable->findColKey(LT::NoThrow,IDeA_KEY(BASISSWAP,ENDDATE).getName());
								if(k4 == LT::Table::not_found)
								{
									k4 = basisSwapTable->findColKey(IDeA_KEY(BASISSWAP,ENDDATE).getAliases());
								}
							}

							for(size_t i = 1; i < basisSwapTable->rowsGet(); ++i)
							{
								LT::Str description = basisSwapTable->at(i,k4);
								if( !description.empty() )
								{
									double spread = basisSwapTable->at(i,k3);
									(*it)->setRate(spread);
									++it;
								}
							}
						}
					}
				}
			}
			else if(turns.compareCaseless(instrumentName) != 0 && steps.compareCaseless(instrumentName) != 0 && bumps.compareCaseless(instrumentName) != 0)
			{
				GenericDataPtr instrumentSubTable(instrumentsTable->get<GenericDataPtr>(i, 1));
			
				size_t col = LT::Table::not_found, cvxCol = LT::Table::not_found;
				col = instrumentSubTable->table->findColKey(LT::NoThrowT(), "RATE");
				cvxCol = instrumentSubTable->table->findColKey(LT::NoThrowT(), "CONVEXITY");
				if(col == LT::Table::not_found) 
				{
					col = instrumentSubTable->table->findColKey(LT::NoThrowT(), "SPREAD");
					if(col == LT::Table::not_found) 
					{
						col = instrumentSubTable->table->findColKey(LT::NoThrowT(), "PRICE");
					}
				}
				
				for(size_t j = 1; j <= IDeA::numberOfRecords(*instrumentSubTable); ++j, ++it)
				{
					double rate = instrumentSubTable->get<double>(col,j);
					if((*it)->getName().compareCaseless(Futures::getName()) == 0)
					{
						if(cvxCol != LT::Table::not_found)
						{
							double cvx = instrumentSubTable->get<double>(cvxCol,j);
							(*it)->setConvexity(cvx);
						}
						(*it)->setRate(1.0 - rate);
					}
					else
						(*it)->setRate(rate);
				}
			}
			
        }
    }

    void CalibrationInstrumentFactory::updateInstrumentList(CalibrationInstruments& instruments,
															LTQuant::GenericDataPtr data)
    {
        size_t instrumentIndex(0);
		GenericDataPtr instrumentsTable(IDeA::extract<GenericDataPtr>(*data, getKey<CalibrationInstruments>()));
	
        for(size_t i(0); i < instrumentsTable->numTags(); ++ i)
        {
            string instrumentName(instrumentsTable->get<string>(i, 0));
            GenericDataPtr instrumentSubTable(instrumentsTable->get<GenericDataPtr>(i, 1));
            
            CalibrationInstrumentFactory::create(instrumentName).second(instruments, instrumentSubTable, &instrumentIndex);
        }
    }



    CalibrationInstrumentFactory* const CalibrationInstrumentFactory::instance()
    {
        static CalibrationInstrumentFactory theInstance;
        return &theInstance;
    }

	namespace details
	{
		void doNothing1(FlexYCF::CalibrationInstruments& /* instrumentList */, 
					   LTQuant::GenericDataPtr /* instrumentTable */, 
					   LTQuant::GenericDataPtr /* data */, 
					   FlexYCF::GlobalComponentCache&, 
					   const LTQuant::PriceSupplierPtr)
		{
		}

		void doNothing2(FlexYCF::CalibrationInstruments& /* instrumentList */, 
						LTQuant::GenericDataPtr /* instrumentTable */, 
						size_t* /* instrumentIndex */)
		{
		}

		FlexYCF::CalibrationInstrumentFunctions doNothingFunctions()
		{
			return std::make_pair(*doNothing1, *doNothing2);
		}
	}
}
