/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "EquivalentCurveCalculator.h"
#include "EquivalentCurveFunctionsFactory.h"
#include "CalibrationInstruments.h"
#include "GlobalComponentCache.h"
#include "CalibrationInstrumentFactory.h"
#include "CalibrationInstruments.h"
#include "BaseModelFactory.h"
#include "BaseModel.h"
#include "Futures.h"
#include "TenorBasisSwap.h"
#include "StructureInstrumentUtils.h"
#include "FlexYCFZeroCurve.h"

//	LTQuantLib
#include "Maths/LeastSquaresProblem.h"
#include "Maths/LeastSquaresSolverFactory.h"
#include "Maths/LeastSquaresSolver.h"
#include "Maths/LevenbergMarquardtSolver.h"
#include "Data/GenericData.h"
#include "Pricers/ZeroCurve.h"
#include "LTQuantUtils.h"

// Standard
#include <vector>
#include <string>

// IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"
#include "YieldCurveIF.h"
#include "QLYieldCurve.h"

using namespace LTQC;
using namespace LTQuant;

namespace 
{

	// move to TenorBasisSwap files?

	// The conversion with convertToGenericData will create an empty table if no values are
	//	provided and this will throw an exception in the TenorBasisSwap static constructor.
	// So we have to ensure it has the right dimensions.
	void adjustTbsValuesTableIfNecessary(const LTQuant::GenericDataPtr& tenorBasisSwapsTable)
	{
		if(!FlexYCF::TenorBasisSwap::hasNewMarketDataFormat(tenorBasisSwapsTable))
		{
			const LTQuant::GenericDataPtr& maturitiesTable(IDeA::extract<LTQuant::GenericDataPtr>(*tenorBasisSwapsTable, IDeA_KEY(TENORBASISSWAPDETAILS, MATURITY)));
			const LTQuant::GenericDataPtr& tenorsTable(IDeA::extract<LTQuant::GenericDataPtr>(*tenorBasisSwapsTable, IDeA_KEY(TENORBASISSWAPDETAILS, TENOR)));
			const LTQuant::GenericDataPtr& valuesTable(IDeA::extract<LTQuant::GenericDataPtr>(*tenorBasisSwapsTable, IDeA_KEY(TENORBASISSWAPDETAILS, VALUES)));

			const size_t nbMaturities(IDeA::numberOfRecords(*maturitiesTable));
			const size_t nbTenors(IDeA::numberOfRecords(*tenorsTable));
		
			// If the conditions to met in TenorBasisSwaps static constructor 
			//	are not verified, fill the whole table with zeroes
			if(valuesTable->numItems() != nbMaturities || valuesTable->numTags() != nbTenors)
			{
				for(size_t mat(0); mat < nbMaturities; ++mat)
				{
					for(size_t tenor(0); tenor < nbTenors; ++tenor)
					{
						valuesTable->set<double>(tenor, mat, 0.0);
					}
				}
			}
		}
		else
		{
			LT::TablePtr table = tenorBasisSwapsTable->table;
			size_t rows = table->rowsGet();
			size_t k1 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, TENOR).getName());
			size_t k2 = table->findColKey(IDeA_KEY(TENORBASISSWAPBYTENOR, BASISSWAP).getName());
			
			for(size_t row = 1; row < rows; ++row)
			{
				LT::Str tenorStr = table->at(row,k1);
				if(!tenorStr.empty())
				{
					LT::TablePtr basisSwapTable = table->at(row,k2);
					size_t k = basisSwapTable->findColKey(IDeA_KEY(BASISSWAP,RATE).getName());
					for(size_t i = 1; i < basisSwapTable->rowsGet(); ++i)
					{
						basisSwapTable->at(i,k) = LTQuant::ConvertToCell<double>(0.0);
					}
				}
			}
		}
	}

	class EqCurveArgs
	{
	public:
		EqCurveArgs(const std::string& instrumentType,
					const LTQuant::GenericDataPtr& instrumentTable,
					const FlexYCF::InstrumentCollection& instruments):
			m_instrumentType(instrumentType),
			m_instrumentTable(instrumentTable),
			m_instruments(instruments)
		{
		}

		inline const std::string& instrumentType() const								{ return m_instrumentType;	}
		inline const LTQuant::GenericDataPtr& instrumentTable() const					{ return m_instrumentTable;	}
		inline const FlexYCF::InstrumentCollection& calibrationInstruments() const		{ return m_instruments;		}

	private:
		std::string							m_instrumentType;
		LTQuant::GenericDataPtr				m_instrumentTable;
		FlexYCF::InstrumentCollection		m_instruments;
	};

    void calculateEquivalentCurveInt(const LTQuant::PriceSupplierPtr& ps,
                                     const FlexYCF::BaseModelPtr& model,
								     const LTQuant::GenericDataPtr& targetCurveTable)
	{
        using namespace FlexYCF;

		// Load the instruments from the target curve table
		CalibrationInstruments instruments;
		
		// The algorithm uses a mapping between an instrument type name
		//	and the corresponding table that represents a set of instruments
		//	of this type only
		// Note: we do not just rely on table name as it can be different from the
		//	instrument type name for which they store information
		typedef std::vector<EqCurveArgs> InstrumentTables;
		InstrumentTables instrumentTables;

		const LTQuant::GenericDataPtr instrumentListTable(IDeA::extract<LTQuant::GenericDataPtr>(*targetCurveTable, IDeA_KEY(YIELDCURVE,YC_INSTRUMENTLIST)));
		
		// Create a global component cache
		GlobalComponentCache gcc;
		
		// Set it for global access (otherwise the program may crash
		// when calling getGlobalComponentCache())
		InstrumentComponent::setGlobalComponentCache(&gcc);

		std::string instrumentType;
		LTQuant::GenericDataPtr instrumentTable;
	
		StringVector tbsAliases;
		getAliases<TenorBasisSwap>(tbsAliases);

		// Iterate through each instrument type in the instrument list
		for(size_t cnt(0); cnt < instrumentListTable->numTags(); ++cnt)
		{
			instrumentType  = instrumentListTable->get<std::string>(cnt, 0);
				
			// No need to keep a pointer to the tables for turns, steps or bumps instruments:
			//	they are not even represented as FlexYCF::CalibrationInstrument's !
			if(!isTurnsOrStepsOrBumpsName(instrumentType))
			{
				instrumentTable = instrumentListTable->get<LTQuant::GenericDataPtr>(cnt, 1);

				//	if(instrumentType == TenorBasisSwap::getName())
				if(doesCollectionContain(tbsAliases, LT::Str(instrumentType)))
				{
					// Adjust the TenorBasisSwap Values table that has not been converted 
					//	correctly as it has no header
					adjustTbsValuesTableIfNecessary(instrumentTable);
				}

				//	instrumentTables.push_back(make_pair<std::string, LTQuant::GenericDataPtr>(instrumentType, instrumentTable));
				instruments.clear();

				// Population the instruments object with the instruments 
				//	of the same type represented in the instrument table
				// Note: at this stage, their rates/spreads/prices will be 
				//	set to a default value that is going to be overwritten.
				CalibrationInstrumentFactory::create(instrumentType).first(instruments,
																		   instrumentTable,
																		   targetCurveTable,
																		   gcc,
																		   ps);

				// Calculate (and set) the implicit rates of the target curve instruments
				calculateImplicitInstrumentRates(model, instruments);
	
				instrumentTables.push_back(EqCurveArgs(instrumentType, instrumentTable, instruments.getInstrumentCollection()));
			}
		}

		// Populate the target curve table with those rates 
		for(InstrumentTables::const_iterator iter(instrumentTables.begin()); iter != instrumentTables.end(); ++iter)
		{
			EquivalentCurveFunctionsFactory::fillInstrumentsTable(iter->instrumentType(), iter->calibrationInstruments(), iter->instrumentTable());
		}

		// * The EquivalentCurve function ends here *
		// What follows is just to debug and check that the rates are all set:
		/*
		double rateOrPrice;
		const size_t nbTags(instrumentListTable->numTags());
		const size_t nbItems(instrumentListTable->numItems());

		for(size_t tableCnt(0); tableCnt < instrumentListTable->numTags(); ++tableCnt)
		{
			instrumentType  = instrumentListTable->get<std::string>(tableCnt, 0);
			instrumentTable = instrumentListTable->get<LTQuant::GenericDataPtr>(tableCnt, 1);

			const size_t nbTags0(instrumentTable->numTags());
			const size_t nbItems0(instrumentTable->numItems());

			if(!isTurnsOrStepsOrBumpsName(instrumentType))
			{
				if(instrumentTable->doesTagExist("Rate"))
				{
					for(size_t cnt(0); cnt < instrumentTable->numItems() - 1; ++cnt)
					{
						rateOrPrice = instrumentTable->get<double>("Rate", cnt);	// is 0.0 !!
					}
				}
				else if(instrumentTable->doesTagExist("Price"))
				{
					for(size_t cnt(0); cnt < instrumentTable->numItems() - 1; ++cnt)
					{
						rateOrPrice = instrumentTable->get<double>("Price", cnt);
					}
				}
				else
				{
					// TBS
				}
			}
		}
		*/
	}
}

namespace FlexYCF
{
	void calculateEquivalentCurve(const IDeA::YieldCurveIFConstPtr& yieldCurve,
								  const LTQuant::GenericDataPtr& targetCurveTable)
    {
		// Retrieve the model inside the FlexYCF source curve
		const BaseModelPtr model(yieldCurve->getModel());
		if(model == 0)
		{
			LT_THROW_ERROR( "The model in the FlexYCF source curve has not been built" );
		}

		// downcast to a QLYieldCurve
		const IDeA::QLYieldCurveConstPtr qlyc = dynamicCast<const IDeA::QLYieldCurve>(yieldCurve);

        calculateEquivalentCurveInt(qlyc->getPriceSupplier(), model, targetCurveTable);
    }

    void calculateEquivalentCurve(const LTQuant::ZeroCurvePtr& zeroCurve,
								  const LTQuant::GenericDataPtr& targetCurveTable)
	{
		// Downwcast the source curve to a FlexYCFZeroCurve
		const FlexYCFZeroCurvePtr flexZC(std::tr1::dynamic_pointer_cast<FlexYCFZeroCurve>(zeroCurve));
		if(flexZC == 0)
		{
			LT_THROW_ERROR( "The source curve is not a FlexYCFZeroCurve." );
		}

		// Retrieve the model inside the FlexYCF source curve
		const BaseModelPtr model(flexZC->getModel());
		if(model == 0)
		{
			LT_THROW_ERROR( "The model in the FlexYCF source curve has not been built" );
		}

        calculateEquivalentCurveInt(flexZC->getParent(), model, targetCurveTable);
	}


	void calculateImplicitInstrumentRates(const BaseModelPtr& model,
										  const CalibrationInstruments& instruments)
	{		
		for(CalibrationInstruments::const_iterator iter(instruments.begin());
			iter != instruments.end(); ++iter)
		{
			(*iter)->setParRate(model);
		}
	}

}