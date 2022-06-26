/*****************************************************************************
    
	EquivalentCurveFunctionsFactory

	Implementation of the EquivalentCurveFunctionsFactory class.

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "EquivalentCurveFunctionsFactory.h"
#include "CashInstrument.h"
#include "ZeroRate.h"
#include "ZeroSpread.h"
#include "TenorZeroRate.h"
#include "ForwardZeroRate.h"
#include "ForwardRateAgreement.h"
#include "FXForward.h"
#include "Futures.h"
#include "InterestRateSwap.h"
#include "CrossCurrencySwap.h"
#include "TenorBasisSwap.h"
#include "OvernightIndexedSwap.h"
#include "OISBasisSwap.h"
#include "CurveType.h"

//	LTQuantLib
#include "Data/GenericData.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"


using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{
	EquivalentCurveFunctionsFactory::EquivalentCurveFunctionsFactory()
	{
		registerFunctions<CashInstrument>();
        registerFunctions<ZeroRate>();
		registerFunctions<ZeroSpread>();
		registerFunctions<TenorZeroRate>();
		registerFunctions<ForwardZeroRate>();
        registerFunctions<FxForwardInstrument>();
        registerFunctions<ForwardRateAgreement>();
		registerFunctions<Futures>();
		registerFunctions<InterestRateSwap>();
		registerFunctions<CrossCurrencySwap>();
		registerFunctions<TenorBasisSwap>();
        registerFunctions<OvernightIndexedSwap>();
        registerFunctions<OISBasisSwap>();
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Template specializations
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Cash
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<CashInstrument>()
	{
		return IDeA_KEY(CASH, RATE);
	}

	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<CashInstrument>()
	{
		return IDeA_KEY(CASH, TENOR);
	}
    
    // ZeroRate
    template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<ZeroRate>()
	{
		return IDeA_KEY(ZERORATE, RATE);
	}
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<ZeroRate>()
	{
		return IDeA_KEY(ZERORATE, TENOR);
	}
	// ZeroSpread
    template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<ZeroSpread>()
	{
		return IDeA_KEY(ZEROSPREAD, RATE);
	}
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<ZeroSpread>()
	{
		return IDeA_KEY(ZEROSPREAD, TENOR);
	}
	// ForwardZeroRate
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<ForwardZeroRate>()
	{
		return IDeA_KEY(FORWARDZEROSPREAD, RATE);
	}
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<ForwardZeroRate>()
	{
		return IDeA_KEY(FORWARDZEROSPREAD, STARTDATE);	
	}
    
    //	FXForward
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<FxForwardInstrument>()
	{
		return IDeA_KEY(FXFORWARD, RATE);
	}

	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<FxForwardInstrument>()
	{
		return IDeA_KEY(FXFORWARD, TENOR);
	}

    //	FRA
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<ForwardRateAgreement>()
	{
		return IDeA_KEY(FRA, RATE);
	}

	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<ForwardRateAgreement>()
	{
		return IDeA_KEY(FRA, DESCRIPTION);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Futures	- not need for maturity key specialization
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<Futures>()
	{
		return IDeA_KEY(FUTURE, PRICE);
	}

	template<>
	void EquivalentCurveFunctionsFactory::fillInstrumentTable<Futures>(const FlexYCF::InstrumentCollection& instruments,
																	   LTQuant::GenericData& futuresTable)
	{
		const size_t nbFutures(IDeA::numberOfRecords(futuresTable));

		if(IDeA::keyExists(futuresTable, IDeA_KEY(FUTURE, PRICE)))
		{	
			if(instruments.size() == nbFutures)
			{
				for(size_t cnt(0); cnt < instruments.size(); ++cnt)
				{
					IDeA::inject<double>(futuresTable, IDeA_KEY(FUTURE, PRICE), cnt, 1.0 - instruments[cnt]->getRate());
				}
			}
			else if(instruments.size() < nbFutures)
			{
				size_t futuresCnt(0);
				string description;
				const string empty("");
				for(size_t row(0); row < nbFutures; ++row)
				{
					IDeA::permissive_extract<std::string>(futuresTable, IDeA_KEY(FUTURE, DESCRIPTION), row, description, empty);
					
					if(description != empty)
					{
						IDeA::inject<double>(futuresTable, IDeA_KEY(FUTURE, PRICE), row, 1.0 - instruments[futuresCnt++]->getRate());
					}
				}
			}
			else
			{
				LT_THROW_ERROR( "The number of CalibrationInstrument objects is not equal to the number of instruments in the table." );
			}
		}
		else
		{
			LT_THROW_ERROR( "Tag 'Price' does no exist." );
		}
	}

	//	Note: generic implementation of copyInstrumentTable for Futures is fine
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Swaps
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<InterestRateSwap>()
	{
		return IDeA_KEY(SWAP, RATE);
	}

	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<InterestRateSwap>()
	{
		return IDeA_KEY(SWAP, TENOR);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Cross currency swaps
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<CrossCurrencySwap>()
	{
		return IDeA_KEY(CCYBASIS, SPREAD);
	}

	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<CrossCurrencySwap>()
	{
		return IDeA_KEY(CCYBASIS, TENOR);
	}

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	OIS
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<OvernightIndexedSwap>()
	{
		return IDeA_KEY(OIS, RATE);
	}

	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<OvernightIndexedSwap>()
	{
		return IDeA_KEY(OIS, TENOR);
	}
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	OISBasis
	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getRateKey<OISBasisSwap>()
	{
		return IDeA_KEY(OISBASIS, RATE);
	}

	template<>
	const IDeA::DictionaryKey& EquivalentCurveFunctionsFactory::getMaturityKey<OISBasisSwap>()
	{
		return IDeA_KEY(OISBASIS, TENOR);
	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Tenor basis swaps - not need for key specialization
	template<>
	void EquivalentCurveFunctionsFactory::fillInstrumentTable<TenorBasisSwap>(const FlexYCF::InstrumentCollection& instruments,
																			  LTQuant::GenericData& instrumentTable)
	{
		if(!FlexYCF::TenorBasisSwap::hasNewMarketDataFormat(instrumentTable.table))
		{
			const LTQuant::GenericDataPtr& maturitiesTable(IDeA::extract<GenericDataPtr>(instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY)));
			const LTQuant::GenericDataPtr& tenorsTable(IDeA::extract<GenericDataPtr>(instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR)));
			const LTQuant::GenericDataPtr& valuesTable(IDeA::extract<GenericDataPtr>(instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES)));

			const size_t nbMaturities(IDeA::numberOfRecords(*maturitiesTable));
			size_t nbTenors(IDeA::numberOfRecords(*tenorsTable));
		
			// The number of instruments and the size of the 'Values' table should perfectly match
			if( instruments.size() != nbTenors * nbMaturities )
			{
				LT_THROW_ERROR( "The number of tenor basis swaps does not match the number of quotes in the table" );
			}

			size_t index(0);
			// assume the tenor basis swaps are sorted in this order,
			//	which should be the case
			for(size_t mat(0); mat < nbMaturities; ++mat)
			{
				for(size_t tenor(0); tenor < nbTenors; ++tenor)
				{
					valuesTable->set<double>(tenor, mat, instruments[index++]->getRate());
				}
			}
		}
		else
		{
			LT::TablePtr table = instrumentTable.table;
			size_t rows = table->rowsGet();
			size_t k1 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
			size_t k2 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());
			size_t index = 0;
			for(size_t row = 1; row < rows; ++row)
			{
				LT::Str tenorStr = table->at(row,k1);
				if(!tenorStr.empty())
				{
					LT::TablePtr basisSwapTable = table->at(row,k2);
					size_t k = basisSwapTable->findColKey(IDeA_KEY(BASISSWAP,RATE).getName());
					for(size_t i = 1; i < basisSwapTable->rowsGet(); ++i)
					{
						basisSwapTable->at(i,k) = instruments[index++]->getRate();
					}
				}
			}
		}
	}

	template<>
	void EquivalentCurveFunctionsFactory::copyInstrumentTable<TenorBasisSwap>(const LTQuant::GenericData& instrumentData,
																			  LT::Table& instrumentTable)
	{
		if(!FlexYCF::TenorBasisSwap::hasNewMarketDataFormat(instrumentData.table))
		{
			const LTQuant::GenericDataPtr& maturitiesData(IDeA::extract<GenericDataPtr>(instrumentData, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY)));
			const LTQuant::GenericDataPtr& tenorsData(IDeA::extract<GenericDataPtr>(instrumentData, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR)));
			const LTQuant::GenericDataPtr& valuesData(IDeA::extract<GenericDataPtr>(instrumentData, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES)));

			const size_t nbMaturities(IDeA::numberOfRecords(*maturitiesData));
			const size_t nbTenors(IDeA::numberOfRecords(*tenorsData));

			const LT::TablePtr& valuesTable(extract(instrumentTable.at(1, LT::Str("Values"))));
	
			// dimensions of the table may be 0 x 0 if the cells are left blank
			valuesTable->resize(nbMaturities, nbTenors);

			for(size_t mat(0); mat < nbMaturities; ++mat)
			{
				for(size_t tenor(0); tenor < nbTenors; ++tenor)
				{
					valuesTable->at(mat, tenor) = valuesData->get<double>(tenor, mat);
				}
			}
		}
		else
		{
			LT::TablePtr table = instrumentData.table;
			size_t rows = table->rowsGet();
			size_t k1 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
			size_t k2 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());
			
			size_t rowsInstr = instrumentTable.rowsGet();
			size_t k1Instr   = instrumentTable.findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
			size_t k2Instr   = instrumentTable.findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());
			
			
			for(size_t row = 1; row < rows; ++row)
			{
				LT::Str tenorStr = table->at(row,k1);
				if(!tenorStr.empty())
				{
					LT::TablePtr basisSwapTable = table->at(row,k2);
					LT::TablePtr basisSwapInstrumentTable = instrumentTable.at(row,k2Instr);
					size_t k = basisSwapTable->findColKey(IDeA_KEY(BASISSWAP,RATE).getName());
					size_t kInstr = basisSwapInstrumentTable->findColKey(IDeA_KEY(BASISSWAP,RATE).getName());
					for(size_t i = 1; i < basisSwapTable->rowsGet(); ++i)
					{
						basisSwapInstrumentTable->at(i,kInstr) = basisSwapTable->at(i,k);
					}
				}
			}
		}
	}

	template<>
	void EquivalentCurveFunctionsFactory::fillInstrumentTable<TenorZeroRate>(const FlexYCF::InstrumentCollection& instruments,
																			  LTQuant::GenericData& instrumentTable)
	{
		
		LT::TablePtr table = instrumentTable.table;
		size_t rows = table->rowsGet();
		size_t k1 = table->findColKey(IDeA_KEY(TENORZERORATES, TENOR).getName());
		size_t k2 = table->findColKey(IDeA_KEY(TENORZERORATES, ZERORATE).getName());
		size_t index = 0;
		for(size_t row = 1; row < rows; ++row)
		{
			LT::Str tenorStr = table->at(row,k1);
			if(!tenorStr.empty())
			{
				LT::TablePtr zeroTable = table->at(row,k2);
				size_t k = zeroTable->findColKey(IDeA_KEY(ZERORATE,RATE).getName());
				for(size_t i = 1; i < zeroTable->rowsGet(); ++i)
				{
					zeroTable->at(i,k) = instruments[index++]->getRate();
				}
			}
		}
	}

	template<>
	void EquivalentCurveFunctionsFactory::copyInstrumentTable<TenorZeroRate>(const LTQuant::GenericData& instrumentData,
																			  LT::Table& instrumentTable)
	{
		
		LT::TablePtr table = instrumentData.table;
		size_t rows = table->rowsGet();
		size_t k1 = table->findColKey(IDeA_KEY(TENORZERORATES, TENOR).getName());
		size_t k2 = table->findColKey(IDeA_KEY(TENORZERORATES, ZERORATE).getName());
			
		size_t rowsInstr = instrumentTable.rowsGet();
		size_t k1Instr   = instrumentTable.findColKey(IDeA_KEY(TENORZERORATES, TENOR).getName());
		size_t k2Instr   = instrumentTable.findColKey(IDeA_KEY(TENORZERORATES, ZERORATE).getName());
			
			
		for(size_t row = 1; row < rows; ++row)
		{
			LT::Str tenorStr = table->at(row,k1);
			if(!tenorStr.empty())
			{
				LT::TablePtr zeroTable = table->at(row,k2);
				LT::TablePtr zeroInstrumentTable = instrumentTable.at(row,k2Instr);
				size_t k = zeroTable->findColKey(IDeA_KEY(ZERORATE,RATE).getName());
				size_t kInstr = zeroInstrumentTable->findColKey(IDeA_KEY(ZERORATE,RATE).getName());
				for(size_t i = 1; i < zeroTable->rowsGet(); ++i)
				{
					zeroInstrumentTable->at(i,kInstr) = zeroTable->at(i,k);
				}
			}
		}
	}

	void EquivalentCurveFunctionsFactory::fillInstrumentsTable(const std::string& instrumentTypeName,
															   const InstrumentCollection& instruments,
															   const LTQuant::GenericDataPtr& instrumentsTable)
	{
		EquivalentCurveFunctionsFactory::create(instrumentTypeName).first(instruments, *instrumentsTable);
	}

	void EquivalentCurveFunctionsFactory::copyMarketData(const std::string& instrumentTypeName,
													     const LTQuant::GenericDataPtr& instrumentsData,
													     const LT::TablePtr& instrumentsTable)
	{
		if(IDeA::numberOfRecords(*instrumentsData) != IDeA::numberOfRecords(*instrumentsTable))
		{
			LT_THROW_ERROR( "The instrument table for " << instrumentTypeName << " does not have the same number as the corresponding instrument data" );
		}
		
		EquivalentCurveFunctionsFactory::create(instrumentTypeName).second(*instrumentsData, *instrumentsTable);
	}

	EquivalentCurveFunctionsFactory* const EquivalentCurveFunctionsFactory::instance()
	{
		static EquivalentCurveFunctionsFactory theInstance;
		return &theInstance;
	}

}
