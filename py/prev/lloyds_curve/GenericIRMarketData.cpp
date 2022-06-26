/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

#include "stdafx.h"
#include "GenericIRMarketData.h"
#include "FlexYCFZeroCurve.h"
#include "YieldCurveCreator.h"
#include "DateUtils.h"
#include "CalibrationInstrument.h"
#include "CashInstrument.h"
#include "Futures.h"
#include "InterestRateSwap.H"
#include "CrossCurrencySwap.h"
#include "TenorBasisSwap.h"

#include "DictYieldCurve.h"
#include "DataExtraction.h"
#include "TradeConventionCalcH.h"

#include "RollConv.h"

#include "Library/PublicInc/Date.h"

// ModuleStaticData
#include "ModuleStaticData/InternalInterface/IRIndexProperties.h"


using namespace LTQC;
using namespace std;

namespace LTQuant
{
    GenericIRMarketData::GenericIRMarketData(
                            LT::date valueDate, 
                            const Index& index) :
        IRMarketData(valueDate, index),
        m_builtPoints(false)
    {
    }
    
    GenericIRMarketData::GenericIRMarketData(const GenericIRMarketData& other) :
        IRMarketData(other.getValueDate(), other.getIndex()),
        m_builtPoints(false)
    {
        setData(other.getData());
    }
    
    GenericIRMarketData::GenericIRMarketData()
    {
    }
    
    GenericIRMarketData::~GenericIRMarketData()
    {
    }
/**---------------------------------------------------------------------------------------------------------------*
    
    Method      : GenericIRMarketData::getModelName

    Description : Returns the FLEXYC model name

*----------------------------------------------------------------------------------------------------------------*/
    const std::string &GenericIRMarketData::getModelName() const
    { 
        static std::string model("Interpolation:FLEXYC:YieldCurve"); 
        return model; 
    }
/**---------------------------------------------------------------------------------------------------------------*
    
    Method      : GenericIRMarketData::addToPricer

    Description : Builds the yield curve and adds it to the price supplier

                  NB -  Arjun has made changes to cater for different swaption properties at different 
                  maturities. He utilises the index properties to do this. In theory he should allow
                  the settings on the yield curve to override this but certain FlexYCF unit tests
                  fail on this as they do not have properly constructed curves embedded in them. LEER

*----------------------------------------------------------------------------------------------------------------*/    
    void GenericIRMarketData::addToPricer(PriceSupplierPtr priceSupplier, string constructionMethod)
    {
        using namespace ModuleStaticData;

        if(getValueDate() != priceSupplier->getDate())
        {
            LT_THROW_ERROR("Can't add with inconsistent dates, market data is " << getValueDate() << " Price supplier is " << priceSupplier->getDate());
        }

        // check our defaults to see where we should be resolving them from.
        IRIndexPropertiesPtr indexProps(getIRIndexProperties(getCurrency() + getIndexName()));

        ZeroCurvePtr curve(new FlexYCFZeroCurve(priceSupplier.get(), shared_from_this(), constructionMethod, indexProps));
        priceSupplier->addZeroCurve(getCurrency(), getIndexName(), curve);
    }

    size_t GenericIRMarketData::getNumberOfSections() const
    {
		// force a syncronization
		// if not already done
		if(!m_builtPoints)
		{
			copyDataFromTable();
			m_builtPoints = true;
		}

        return m_sectionTypes.size();
    }
    
    MarketData::eSectionType GenericIRMarketData::getSectionType(size_t sectionIndex) const
    {
		// force a syncronization
		// if not already done
		if(!m_builtPoints)
		{
			copyDataFromTable();
			m_builtPoints = true;
		}

        if(sectionIndex >= m_sectionTypes.size())
        {
			LT_THROW_ERROR("Invalid section requested");
		}

		if(m_sectionTypes[sectionIndex] == TENOR_SPREAD_TYPE)
		{
			return DATE_DOUBLE_GRID;
		}
		else
		{
			return LIST;
		}
    }

    const MarketData::PointList& GenericIRMarketData::getList(size_t sectionIndex) const
    {
		// force a syncronization
		// if not already done
		if(!m_builtPoints)
		{
			copyDataFromTable();
			m_builtPoints = true;
		}

		if(sectionIndex >= m_sectionTypes.size())
        {
            LT_THROW_ERROR("Invalid section requested");
        }
        if(m_sectionTypes[sectionIndex] == TENOR_SPREAD_TYPE)
        {
            LT_THROW_ERROR("Invalid section requested");
        }

		return m_pointLists[sectionIndex];
    }

    MarketData::PointList& GenericIRMarketData::getList(size_t sectionIndex)
    {
		// force a syncronization
		// if not already done
		if(!m_builtPoints)
		{
			copyDataFromTable();
			m_builtPoints = true;
		}

		if(sectionIndex >= m_sectionTypes.size())
        {
            LT_THROW_ERROR("Invalid section requested");
        }
        if(m_sectionTypes[sectionIndex] == TENOR_SPREAD_TYPE)
        {
            LT_THROW_ERROR("Invalid section requested");
        }

		return m_pointLists[sectionIndex];
	}

	const MarketData::DateDoublePointGrid& GenericIRMarketData::getDateDoubleGrid(size_t sectionIndex) const
    {
		// force a syncronization
		// if not already done
		if(!m_builtPoints)
		{
			copyDataFromTable();
			m_builtPoints = true;
		}

		if(sectionIndex >= m_sectionTypes.size())
        {
            LT_THROW_ERROR("Invalid section requested");
        }
        if(m_sectionTypes[sectionIndex] != TENOR_SPREAD_TYPE)
        {
            LT_THROW_ERROR("Invalid section requested");
        }

		return m_pointGrids[sectionIndex];
    }
    
    MarketData::DateDoublePointGrid& GenericIRMarketData::getDateDoubleGrid(size_t sectionIndex)
    {
		// force a syncronization
		// if not already done
		if(!m_builtPoints)
		{
			copyDataFromTable();
			m_builtPoints = true;
		}

		if(sectionIndex >= m_sectionTypes.size())
        {
            LT_THROW_ERROR("Invalid section requested");
        }
        if(m_sectionTypes[sectionIndex] != TENOR_SPREAD_TYPE)
        {
            LT_THROW_ERROR("Invalid section requested");
        }

        return m_pointGrids[sectionIndex];
    }

	double GenericIRMarketData::getBlipSignMultiplier(size_t sectionIndex) const
	{
		// force a syncronization
		// if not already done
		if(!m_builtPoints)
		{
			copyDataFromTable();
			m_builtPoints = true;
		}

		if(sectionIndex >= m_sectionTypes.size())
        {
	        LT_THROW_ERROR("Invalid section requested");
        }
        return m_blipSigns[sectionIndex];
	}

    GenericIRMarketDataPtr GenericIRMarketData::clone() const
    {
        GenericIRMarketDataPtr retVal(new GenericIRMarketData(*this));
        return retVal;
    }

    IRMarketData::eIRSectionType GenericIRMarketData::getIRSectionType(size_t sectionIndex) const
    {
		// force a syncronization
		// if not already done
		if(!m_builtPoints)
		{
			copyDataFromTable();
			m_builtPoints = true;
		}

		if(sectionIndex >= m_sectionTypes.size())
        {
	        LT_THROW_ERROR("Invalid section requested");
        }
        return m_sectionTypes[sectionIndex];
    }

	void addPointsToList(const GenericData& instrumentTable, std::vector<MarketData::PointList>& pointLists, const IDeA::DictionaryKey& desckey, const IDeA::DictionaryKey& ratekey, size_t i, const LT::date& startDate, const LT::date& endDate)
	{
		size_t numInstrumentTags(instrumentTable.numItems());
		if(numInstrumentTags > 1)
		{
			pointLists[i].resize(numInstrumentTags - 1);
			for(size_t j(0); j < numInstrumentTags - 1; ++j)
			{
				string dateDesc = IDeA::extract<std::string>(instrumentTable, desckey, j);
				double rate = IDeA::extract<double>(instrumentTable, ratekey, j);
				pointLists[i][j] = MarketDataPoint(dateDesc, rate, 0.0, startDate, endDate);
			}
		}
		else
		{
			pointLists[i].clear();
		}
	}

	/*void GenericIRMarketData::populatePointsList(std::vector<double>& blipSign, std::vector<MarketData::PointList>& pointLists, const GenericData& instrumentListTable, const date& startDate, const date& endDate)
	{
		size_t numInstrumentTypes(instrumentListTable.numTags());

		std::vector<IDeA::DictionaryKey> descriptionKeyList;
		std::vector<IDeA::DictionaryKey> rateKeyList;

        for(size_t i(0); i < numInstrumentTypes; ++i)
        {
			string instrumentName(instrumentListTable.get<string>(i, 0));

			if isSameKey((IDeA_KEY(YC_INSTRUMENTLIST, CASH), )
			// if instrumentName is a valid name of cash
				// add cash desc key
				// add cash rate key
				// record blip
			// else if instrumentName is a valid name of future
				// add future desc key
				// add future rate key
				// record blip
			// else if instrument name is a valid name of swaps
				// add swaps desc key
				// add swaps rate key
				// record blip
			// else if instrument name is a valid name of ccy swaps
				// add ccy swaps desc key
				// add ccy swaps rate key
				// record blip
		}

		// for each key
			// addPointsToList( instrumentTable, point list[i], , deskKey, rateKey)



	}*/
    void GenericIRMarketData::copyDataFromTable() const
    {
        using namespace ModuleStaticData;
        
        // get the data from the parent object (to avoid infinite loop)
		GenericDataPtr data(MarketData::getData());
		if (!data)
			LTQC_THROW(IDeA::DataException, "No data in yield curve");

		GenericDataPtr instrumentListTable = IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST));
		if (!instrumentListTable)
			LTQC_THROW(IDeA::DataException, "No instrument list in yield curve");

		size_t numInstrumentTypes(instrumentListTable->numTags());
        m_sectionTypes.resize(numInstrumentTypes);
        m_pointLists.resize(numInstrumentTypes);
        m_pointGrids.resize(numInstrumentTypes);
		m_blipSigns.resize(numInstrumentTypes);

		GenericDataPtr instrumentTable;
		GenericDataPtr nullTable;
		size_t i = 0;
		bool found = false;
		// CASH
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, CASH), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			m_blipSigns[i] = 1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(CASH,TENOR), IDeA_KEY(CASH, RATE), i, getValueDate(), getValueDate());
			i++;
		}
		// FUTURES
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, FUTURES), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			m_blipSigns[i] = -1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(FUTURE,EXPIRY), IDeA_KEY(FUTURE, PRICE), i, getValueDate(), getValueDate());
			i++;
		}
        // FRA
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, FRA), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			m_blipSigns[i] = 1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(FRA,DESCRIPTION), IDeA_KEY(FRA, RATE), i, getValueDate(), getValueDate());
			i++;
		}
		// SWAPS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, SWAPS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			m_blipSigns[i] = 1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(SWAP,TENOR), IDeA_KEY(SWAP, RATE), i, getValueDate(), getValueDate());
			i++;
		}
		// CCYSWAPS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, CCYBASIS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::SPREAD_TYPE;
			m_blipSigns[i] = 1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(CCYBASIS,TENOR), IDeA_KEY(CCYBASIS, SPREAD), i, getValueDate(), getValueDate());
			i++;
		}
		// TUNRNS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, TURNS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::TURNS_TYPE;
			m_blipSigns[i] = 1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(TURN,DATE), IDeA_KEY(TURN, SPREAD), i, getValueDate(), getValueDate());
			i++;
		}
		// STEPS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, STEPS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::TURNS_TYPE;
			m_blipSigns[i] = 1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(STEP,DATE), IDeA_KEY(STEP, SPREAD), i, getValueDate(), getValueDate());
			i++;
		}
		// BUMPS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, BUMPS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::TURNS_TYPE;
			m_blipSigns[i] = 1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(BUMP,DATE), IDeA_KEY(BUMP, SPREAD), i, getValueDate(), getValueDate());
			i++;
		}
		// ILZCSWAPS
		found = IDeA::permissive_extract<GenericDataPtr>(instrumentListTable, IDeA_KEY(INSTRUMENTLIST, ILZCSWAP), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			m_blipSigns[i] = 1.0;	
			m_pointGrids[i].clear();
			addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(ILZCSWAPCURVEINSTRUMENT,MATURITY), IDeA_KEY(ILZCSWAPCURVEINSTRUMENT, VALUE), i, getValueDate(), getValueDate());
			i++;
		}

        // ZERORATE
        found = IDeA::permissive_extract<GenericDataPtr>(instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, ZERORATE), instrumentTable, nullTable);
        if (found && instrumentTable) {
			LT::TablePtr table = instrumentTable->table;
			bool hasEndDate = false;
			size_t k = table->findColKey(LT::NoThrow,IDeA_KEY(ZERORATE,ENDDATE).getName());
			if(k != LT::Table::not_found)
			{
				hasEndDate = true;
			}
			else
			{
				k = table->findColKey(LT::NoThrow,IDeA_KEY(ZERORATE,ENDDATE).getAliases());
				if(k != LT::Table::not_found)
				{
					hasEndDate = true;
				}
			}

            m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
            m_blipSigns[i] = 1.0;	
            m_pointGrids[i].clear();
			if(hasEndDate)
			{
				addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(ZERORATE,ENDDATE), IDeA_KEY(ZERORATE, RATE), i, getValueDate(), getValueDate());
			}
			else
			{
				addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(ZERORATE,TENOR), IDeA_KEY(ZERORATE, RATE), i, getValueDate(), getValueDate());
			}
			i++;
        }

        // FXFORWARD
        found = IDeA::permissive_extract<GenericDataPtr>(instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, FXFORWARD), instrumentTable, nullTable);
        if (found && instrumentTable) {
            m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
            m_blipSigns[i] = 1.0;	
            m_pointGrids[i].clear();
            addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(FXFORWARD,TENOR), IDeA_KEY(FXFORWARD, RATE), i, getValueDate(), getValueDate());
            i++;
        }

        // COMMODITY FUTURES
        found = IDeA::permissive_extract<GenericDataPtr>(instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, COMMODITYFUTURES), instrumentTable, nullTable);
        if (found && instrumentTable) {
            m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
            m_blipSigns[i] = 1.0;	
            m_pointGrids[i].clear();
            addPointsToList(*instrumentTable, m_pointLists, IDeA_KEY(COMMODITYFUTURES,EXPIRY), IDeA_KEY(COMMODITYFUTURES, PRICE), i, getValueDate(), getValueDate());
            i++;
        }

        // TENOR BASIS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, TENORBASISSWAP), instrumentTable, nullTable);
		if (found && instrumentTable) {

			m_sectionTypes[i] = IRMarketData::TENOR_SPREAD_TYPE;
			m_blipSigns[i] = 1.0;	

			m_pointLists[i].clear();

			LT::date lastEndDate = getValueDate();

			if(!FlexYCF::TenorBasisSwap::hasNewMarketDataFormat(instrumentTable))
			{
				GenericDataPtr maturities = IDeA::extract<GenericDataPtr>(*instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY));
				GenericDataPtr tenors = IDeA::extract<GenericDataPtr>(*instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR));
				GenericDataPtr spreads = IDeA::extract<GenericDataPtr>(*instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES));

				// clear down each time
				m_pointLists[i].clear();

				if (!maturities)
					LTQC_THROW(IDeA::DataException, "No maturities in tenor basis");

				if (!tenors)
					LTQC_THROW(IDeA::DataException, "No tenors in tenor basis");

				if (!spreads)
					LTQC_THROW(IDeA::DataException, "No spreads in tenor basis");

				const size_t numMaturities(maturities->numItems() - 1);
				const size_t numTenors(tenors->numItems() - 1);

				for(size_t k(0); k < numMaturities; ++k)
				{
					std::string dateDesc = IDeA::extract<std::string>(*maturities, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY), k);
					LT::date startDate;
					LT::date endDate;
					const IRIndexPropertiesPtr indexProps(getIRIndexProperties(getIndex().getCurrency() + getIndex().getIndexName()));
					const Date spotDate =  IDeA::TradeConventionCalcH::getSpotDate(Date(getValueDate()), indexProps->getFixingCalendarName(), indexProps->getCalendarName(), LTQC::Tenor(0, indexProps->getSpotDays(), 0, 0, 0));
					LTQC::RollConv mf(LTQC::RollConvMethod::ModifiedFollowing);
					LTQC::RollRuleMethod monthend(LTQC::RollRuleMethod::BusinessEOM);
					Date endDateTmp = IDeA::TradeConventionCalcH::calculateMaturityDate(spotDate, LTQC::Tenor(LT::Str(dateDesc)), indexProps->getCalendarName(), mf, monthend);
					endDate = endDateTmp.getAsLTdate();	            

					for(size_t j(0); j < numTenors; ++j)
					{
						std::string tenorDesc = IDeA::extract<std::string>(*tenors, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR),j);
						const double tenor(ModuleDate::dateDescToYears(tenorDesc) * 12.0);
						double spread(spreads->get<double>(j, k));

						// N.B. FIXME This should be handled by FlexYCF. Why do we need to impose it at this stage?
						if(tenor == 3.0)
						{
							spread = 0;
						}

						const MarketDataPoint point(dateDesc, spread, 0.0, startDate, endDate);
						m_pointGrids[i].add(endDate, tenor, point);
					}
					lastEndDate = endDate;
				}
			}
			else
			{
				LT::TablePtr table = instrumentTable->table;
				size_t rows = table->rowsGet();
				size_t k1 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
				size_t k2 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());
			
				for(size_t row = 1; row < rows; ++row)
				{
					LT::Str tenorStr = table->at(row,k1);
					if(!tenorStr.empty())
					{
						LT::TablePtr basisSwapTable = table->at(row,k2);
						size_t k0 = basisSwapTable->findColKey(IDeA_KEY(BASISSWAP,RATE).getName());
						size_t k1 = basisSwapTable->findColKey(LT::NoThrow, IDeA_KEY(BASISSWAP,TENOR).getName());
						bool hasEndDate = false;
						if(k1 == LT::Table::not_found)
						{
							k1 = basisSwapTable->findColKey(LT::NoThrow,IDeA_KEY(BASISSWAP,ENDDATE).getName());
							if(k1 == LT::Table::not_found)
							{
								k1 = basisSwapTable->findColKey(IDeA_KEY(BASISSWAP,ENDDATE).getAliases());
							}
							hasEndDate = true;
						}

						for(size_t j = 1; j < basisSwapTable->rowsGet(); ++j)
						{

							
							LT::date startDate;
							LT::date endDate;
							const IRIndexPropertiesPtr indexProps(getIRIndexProperties(getIndex().getCurrency() + getIndex().getIndexName()));
							const Date spotDate =  IDeA::TradeConventionCalcH::getSpotDate(Date(getValueDate()), indexProps->getFixingCalendarName(), indexProps->getCalendarName(), LTQC::Tenor(0, indexProps->getSpotDays(), 0, 0, 0));
							LTQC::RollConv mf(LTQC::RollConvMethod::ModifiedFollowing);
							LTQC::RollRuleMethod monthend(LTQC::RollRuleMethod::BusinessEOM);
							if(!hasEndDate)
							{
								LT::Str dateDesc = basisSwapTable->at(j,k1);
								Date endDateTmp = IDeA::TradeConventionCalcH::calculateMaturityDate(spotDate, LTQC::Tenor(LT::Str(dateDesc)), indexProps->getCalendarName(), mf, monthend);
								endDate = endDateTmp.getAsLTdate();
							}
							else
							{
								double dateDesc = basisSwapTable->at(j,k1);
								endDate = Date(static_cast<long>(dateDesc)).getAsLTdate();
							}

							double tenor = 0.0;
							if(!hasEndDate)
							{
								tenor = ModuleDate::dateDescToYears(tenorStr.string()) * 12.0;
							}
							else
							{
								tenor = (endDate.getAsLong() - startDate.getAsLong())/365.0;
							}
							double spread = basisSwapTable->at(j,k0);

							
							LT::Str dateDesc = basisSwapTable->at(j,k1);
							const MarketDataPoint point(dateDesc.string(), spread, 0.0, startDate, endDate);
							m_pointGrids[i].add(endDate, tenor, point);
							lastEndDate = endDate;
						}
					}	
				}
			}
			i++;
		}
    }

	void injectRates(LTQuant::GenericData& instrumentTable, const IDeA::DictionaryKey& tagKey, MarketData::PointList& pointLists)
	{
		size_t numInstrumentTags(instrumentTable.numItems());
		if(numInstrumentTags > 1)
		{
			if(pointLists.size() != numInstrumentTags - 1)
			{
				LT_THROW_ERROR("Inconsistency between instruments and table");
			}
			for(size_t j(0); j < numInstrumentTags - 1; ++j)
			{
				IDeA::inject<double>(instrumentTable, tagKey, j, pointLists[j].getRate());
			}
		}
	}

    // copy the data from the underlying GenericDataPtr object
    // to collections of MarketDataPoint objects so that risk 
    // will work.
    // At present we don't make much of an attempt to get the correct dates
    // in the MarketDataPoints on the basis that risk doesn't use them
    // if we need the correct end dates then we will need to ensure that the code
    // to generate the dates is consistent with the FlexYCF instrument objects
    // preferably by exposing the date generation functions
	void GenericIRMarketData::copyDataBackToTable() const
    {
		// get the data from the parent object (to avoid infinite loop)
		GenericDataPtr data(MarketData::getData());
		if (!data)
			LTQC_THROW(IDeA::DataException, "No data in yield curve");

		GenericDataPtr instrumentListTable = IDeA::extract<GenericDataPtr>(*data, IDeA_KEY(YIELDCURVE, YC_INSTRUMENTLIST));
		if (!instrumentListTable)
			LTQC_THROW(IDeA::DataException, "No instrument list in yield curve");

		size_t numInstrumentTypes(instrumentListTable->numTags());
		if(m_sectionTypes.size() != numInstrumentTypes)
		{
			LT_THROW_ERROR("Inconsistency between instrument list and table");
		}

		GenericDataPtr instrumentTable;
		GenericDataPtr nullTable;
		size_t i = 0;
		bool found = false;
		// CASH
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, CASH), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(CASH, RATE), m_pointLists[i]);
			i++;
		}
		// FUTURES
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, FUTURES), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(FUTURE, PRICE), m_pointLists[i]);
			i++;
		}
        // FRA
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, FRA), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(FRA, RATE), m_pointLists[i]);
			i++;
		}
		// SWAPS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, SWAPS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(SWAP, RATE), m_pointLists[i]);
			i++;
		}
		// CCYSWAPS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, CCYBASIS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::SPREAD_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(CCYBASIS, SPREAD), m_pointLists[i]);
			i++;
		}
		// TUNRNS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, TURNS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::TURNS_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(TURN, SPREAD), m_pointLists[i]);
			i++;
		}
		// STEPS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, STEPS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::TURNS_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(STEP, SPREAD), m_pointLists[i]);
			i++;
		}
		// BUMPS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, BUMPS), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::TURNS_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(BUMP, SPREAD), m_pointLists[i]);
			i++;
		}

		// ILZCSWAPS
		found = IDeA::permissive_extract<GenericDataPtr>(instrumentListTable, IDeA_KEY(INSTRUMENTLIST, ILZCSWAP), instrumentTable, nullTable);
		if (found && instrumentTable) {
			m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
			injectRates(*instrumentTable, IDeA_KEY(ILZCSWAPCURVEINSTRUMENT, VALUE), m_pointLists[i]);
			i++;
		}

        // FXFORWARDS
        found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, FXFORWARD), instrumentTable, nullTable);
        if (found && instrumentTable) {
            m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
            injectRates(*instrumentTable, IDeA_KEY(FXFORWARD, RATE), m_pointLists[i]);
            i++;
        }

        // ZERORATE
        found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, ZERORATE), instrumentTable, nullTable);
        if (found && instrumentTable) {
            m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
            injectRates(*instrumentTable, IDeA_KEY(ZERORATE, RATE), m_pointLists[i]);
            i++;
        }

        // COMMODITYFUTURES
        found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, COMMODITYFUTURES), instrumentTable, nullTable);
        if (found && instrumentTable) {
            m_sectionTypes[i] = IRMarketData::PRIMARY_TYPE;
            injectRates(*instrumentTable, IDeA_KEY(COMMODITYFUTURES, PRICE), m_pointLists[i]);
            i++;
        }


        // TENOR BASIS
		found = IDeA::permissive_extract<GenericDataPtr>(*instrumentListTable, IDeA_KEY(YC_INSTRUMENTLIST, TENORBASISSWAP), instrumentTable, nullTable);
		if (found && instrumentTable) {

			m_sectionTypes[i] = IRMarketData::TENOR_SPREAD_TYPE;
			GenericDataPtr spreads = IDeA::extract<GenericDataPtr>(*instrumentTable, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES));
			if (!spreads)
				LTQC_THROW(IDeA::DataException, "No spreads in tenor basis");

			const vector<LT::date>& rowHeadings(m_pointGrids[i].getRowHeadings());
			const vector<double>& colHeadings(m_pointGrids[i].getColumnHeadings());

			for(size_t row(0); row < rowHeadings.size(); ++row)
			{
				for(size_t col(0); col < colHeadings.size(); ++col)
				{
					const MarketDataPoint& spread(m_pointGrids[i].get(row, col));
					spreads->set(col, row, spread.getRate());
				}
			}

			i++;
		}
    }

    const GenericDataPtr& GenericIRMarketData::getData() const
	{
		if(m_builtPoints)
		{
			copyDataBackToTable();
			m_builtPoints = false;
		}
		return MarketData::getData();
	}
	// *** smelly code warning ***
	// for FlexYCF risk to work with the data being held in the GenericIRMarketData object
	// internally as a GenericData object we need to force sync back to table after blipping
	// otherwise the reset blip is not applied back to the "real" data
    // THIS FUNCTION MUST BE CALLED AFTER DATA MANIPULATIONS
    // it will be called for you if refresh is called on the market data
    // but not 
	// *** smelly code warning ***
	void GenericIRMarketData::syncMarketData()
	{
		if(m_builtPoints)
		{
			copyDataBackToTable();
			m_builtPoints = false;
		}
	}
}

