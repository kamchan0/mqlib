/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "Futures.h"
#include "BaseModel.h"
#include "Data/GenericData.h"
#include "Instruments/Index.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "ForwardRate.h"
#include "GlobalComponentCache.h"
#include "FuturesConvexityModel.h"
#include "Pricers/PriceSupplier.h"
#include "ModuleDate/InternalInterface/Utils.h"
#include "DataExtraction.h"
#include "DateUtils.h"
#include "FlexYcfUtils.h"
#include "ModuleDate/InternalInterface/CalendarFactory.h"
#include "FlexYCFCloneLookup.h"

// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "MarketConvention.h"
#include "TradeConventionCalcH.h"
#include "DictionaryManager.h"
#include "Exception.h"

// QuantCore
#include "utils\DayCount.h"
#include "QCException.h"

// ModuleStaticData
#include "ModuleStaticData/InternalInterface/IRIndexProperties.h"

using namespace LTQC;
using namespace std;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<Futures>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, FUTURES);
	}

    using namespace LTQuant;
    using namespace ModuleDate;

    Futures::Futures(const std::string dateDescription,
                     const LT::date valueDate,
                     const LT::date fixingDate, 
                     const LT::date startDate, 
                     const LT::date endDate, 
                     const double quotedPrice, 
                     const double convexityAdjustment,
                     const DayCounterConstPtr basis,
                     const double contractSize,
                     GlobalComponentCache& globalComponentCache,
                     const FuturesConvexityModelPtr convexityModel):
		CalibrationInstrument(1.0 - quotedPrice, getKeyName<Futures>(), dateDescription, fixingDate, startDate, endDate),
        m_convexityAdjustment(convexityAdjustment),
        m_forwardRate(globalComponentCache.get( ForwardRate::Arguments( valueDate,
																		fixingDate,
                                                                        startDate,
                                                                        endDate,
                                                                        "3M",
                                                                        basis,
                                                                        globalComponentCache))),
        m_timeToExpiry( startDate <= valueDate ? 0.0 : getYearsBetween(valueDate, startDate)),
        m_contractSize( contractSize ),
        m_convexityModel(convexityModel)
    {
    }

    Futures::Futures(const std::string dateDescription,
                     const LT::date valueDate,
                     const LT::date fixingDate, 
                     const LT::date startDate, 
                     const LT::date endDate, 
                     const double quotedPrice, 
                     const double convexityAdjustment,
                     const DayCounterConstPtr basis,
                     const double contractSize):
        CalibrationInstrument(1.0 - quotedPrice, getKeyName<Futures>(), dateDescription, fixingDate, startDate, endDate),
        m_convexityAdjustment(convexityAdjustment),
        m_forwardRate( ForwardRate::create( ForwardRate::Arguments( valueDate,
											                        fixingDate,
                                                                    startDate,
                                                                    endDate,
                                                                    "3M",
                                                                    basis)) ),
        m_timeToExpiry( startDate <= valueDate ? 0.0 : getYearsBetween(valueDate, startDate)),
        m_contractSize( contractSize )
    {
    }

	CalibrationInstrumentPtr Futures::create(const LTQuant::GenericData& instrumentParametersTable, 
											 const LT::Ptr<LT::date>& buildDate,
											 const LTQuant::GenericData& curveParametersTable,
											 const LTQuant::GenericDataPtr&)
	{
        if( !buildDate )
        {
            LTQC_THROW( IDeA::MarketException, "To create a futures instrument, one needs to provide a build date" );
        }
        const LT::date valueDate( *buildDate );
        const std::string description( IDeA::extract< std::string >( instrumentParametersTable, IDeA_KEY(FUTURE, EXPIRY) ) );
        IDeA::IRCurveMktConventionPtr conventions( createIRCurveMktConventions( curveParametersTable ) );
		IDeA::FutureMktConvention& futuresDetails = conventions->m_future;
        DayCounterPtr futuresBasis( LTQC::DayCount::create( futuresDetails.m_dcm ) );
		CalendarPtr futuresCalendar( CalendarFactory::create( futuresDetails.m_calendar.string( ) ) );
        const std::string contractDescriptionPrefix( futuresDetails.m_contractDescriptionPrefix.cStr( ) );
        LT::date startDate, endDate;
        double convexityAdjustment( 0.0 );
        IDeA::permissive_extract< LT::date >( instrumentParametersTable, IDeA_KEY(FUTURE, STARTDATE), startDate, contractDescriptionPrefix.empty( ) ?  getIMMDate( description, futuresCalendar ) : getPrefixedDate( contractDescriptionPrefix + description, futuresCalendar ) );
        IDeA::permissive_extract< LT::date >( instrumentParametersTable, IDeA_KEY(FUTURE, ENDDATE), endDate, addDatePeriod( startDate, futuresDetails.m_rateTenor.asTenorString( ).data( ), futuresCalendar ) );
        IDeA::permissive_extract< double >( instrumentParametersTable, IDeA_KEY(FUTURE, CONVEXITY), convexityAdjustment, 0.0 );
        const LT::date fixingDate = IDeA::IRDepositRateCalcH::calculateFixingDate( Date( startDate ), futuresDetails.m_spotDays, futuresDetails.m_fixingCalendar ).getAsLTdate( );
        const double contractSize( futuresDetails.m_contractSize );
		return CalibrationInstrumentPtr( new Futures( description,
                                                      valueDate,
												      fixingDate,
                                                      startDate,
                                                      endDate,
                                                      0.0,
                                                      convexityAdjustment,
                                                      futuresBasis,
                                                      contractSize )
                                        );
	}

    void Futures::createInstruments(CalibrationInstruments& instruments, 
                                    LTQuant::GenericDataPtr instrumentTable, 
                                    LTQuant::GenericDataPtr masterTable,
                                    GlobalComponentCache& globalComponentCache,
                                    const LTQuant::PriceSupplierPtr priceSupplier)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));

        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));		
		const IDeA::FutureMktConvention& tradeDetails = conventions->m_future;
        const double contractSize( tradeDetails.m_contractSize );

        LTQuant::GenericDataPtr convexityModelTable;
        masterTable->permissive_get<LTQuant::GenericDataPtr>("Convexity Model", 0, convexityModelTable);

        // 2. Create a Futures Convexity Model -- if explicitly specified or if there's a ConvexityModel table, for instance
		const ModuleStaticData::IRIndexPropertiesPtr indexPropertiesForConvexityModel = ModuleStaticData::getIRIndexProperties(LT::Str(tradeDetails.m_currency + tradeDetails.m_index).string());
        FuturesConvexityModelPtr convexityModel(FuturesConvexityModel::create(convexityModelTable, 
                                                                              instrumentTable,
                                                                              valueDate,
                                                                              indexPropertiesForConvexityModel,
                                                                              priceSupplier,
                                                                              globalComponentCache));

        LT_LOG << (static_cast<bool>(convexityModel)? "" : "NOT") << "Using ConvexityModel" << endl;

        std::string const contractDescriptionPrefix(tradeDetails.m_contractDescriptionPrefix.cStr());
        
        // 3. Create all Futures using the convexityModel
		DayCounterPtr futureBasis = LTQC::DayCount::create(tradeDetails.m_dcm);
		CalendarPtr futureCalendar = CalendarFactory::create(tradeDetails.m_calendar.string());
        for(size_t i(0); i < instrumentTable->numItems() - 1; ++i)
        {
			std::string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(FUTURE, EXPIRY), i));

			if(!description.empty())
            {
                const double quotedPrice(IDeA::extract<double>(*instrumentTable, IDeA_KEY(FUTURE, PRICE), i));
                
                LT::date startDate, endDate;
                
				if(instrumentTable->doesTagExist("Start Date"))
				{
					startDate = instrumentTable->get<LT::date>("Start Date", i);
				}
				else
				{
                    // Get the futures contract date. The futures contract description may need prefixing with the currency to indicate that 
                    // currency specific dates are required rather than IMM dates.
                    if (contractDescriptionPrefix.empty())
                        startDate = getIMMDate(description, futureCalendar);
                    else
                        startDate = getPrefixedDate(contractDescriptionPrefix + description, futureCalendar);
					LT_LOG << "Calculating Futures 'Start Date' " << startDate << " from Desc/Expiry " << description;
					
				}

				if(instrumentTable->doesTagExist("End Date"))
				{
					endDate = instrumentTable->get<LT::date>("End Date", i);
				}
				else
				{
					endDate = addDatePeriod(startDate, tradeDetails.m_rateTenor.asTenorString().data(), futureCalendar);				
					LT_LOG << "Calculating Futures 'End Date' " << endDate << " from Desc/Expiry " << description;
					
				}
				LT::date fixingDate = IDeA::IRDepositRateCalcH::calculateFixingDate(Date(startDate), tradeDetails.m_spotDays, tradeDetails.m_fixingCalendar).getAsLTdate();

                double convexityAdjustment(0.0);
                instrumentTable->permissive_get<double>("Convexity", i, convexityAdjustment, 0.0);  // no convexity by default
                
                CalibrationInstrumentPtr instrument( new Futures( description,
                                                                  valueDate,
																  fixingDate,
                                                                  startDate,
                                                                  endDate,
                                                                  quotedPrice,
                                                                  convexityAdjustment,
                                                                  futureBasis,
                                                                  contractSize,
                                                                  globalComponentCache, 
                                                                  convexityModel)
                                                   );
                instruments.add(instrument);
            }
        }
    }

    void Futures::updateInstruments(CalibrationInstruments& instrumentList, 
                                    LTQuant::GenericDataPtr instrumentTable, 
                                    size_t* instrumentIndex)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

        for(size_t i(0); i < instrumentTable->numItems() - 1; ++i)
        {
			const std::string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(FUTURE, EXPIRY), i));
            if(!description.empty())
            {
				const double price(IDeA::extract<double>(*instrumentTable, IDeA_KEY(FUTURE, PRICE), i));
				double convexityAdjustment;
                IDeA::permissive_extract<double>( *instrumentTable, IDeA_KEY(FUTURE, CONVEXITY), i, convexityAdjustment, 0.0);

				instrumentList[*instrumentIndex]->setConvexity(convexityAdjustment);
                instrumentList[*instrumentIndex]->setRate(1.0 - price);
                ++(*instrumentIndex);
            }
        }
    }

    const double Futures::getMarketPrice() const
    {
        return getRate();
    }

    void Futures::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_forwardRate.reset();
        m_convexityModel.reset();
    }

    void Futures::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
        const FuturesPtr ourTypeSrc=std::tr1::static_pointer_cast<Futures>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        m_forwardRate=ourTypeSrc->m_forwardRate;
        m_convexityModel=ourTypeSrc->m_convexityModel;

        CalibrationInstrument::reloadInternalState(src);
    }
    const double Futures::computeModelPrice(const BaseModelPtr model) const
    {
		// for DEBUG
        //double cvxAdj(computeConvexityAdjustment(model));
        //LT_LOG << (*this) << " Rate: " << getRate() << ", Fwd.: " << m_forwardRate->getValue(model) 
        //    << ", Cvx Adj.: " << cvxAdj << ", Fwd+adj: " << (m_forwardRate->getValue(model) + cvxAdj) << endl;
        return m_forwardRate->getValue(*model) + getConvexityAdjustment(model);
    }

    const double Futures::computePV(const BaseModelPtr model) const
    {
        return m_contractSize * m_forwardRate->getTenor( ) * getResidual( model );
    }

    const double Futures::computeBPV(const BaseModelPtr model) const
    {
        return - m_contractSize * m_forwardRate->getTenor( ) * oneBasisPoint( );
    }

    void Futures::accumulateGradient(BaseModel const& model, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {
        m_forwardRate->accumulateGradient(model, multiplier, gradientBegin, gradientEnd);

        /*	Calculating contribution of the convexity spread to the gradient
			is slow and introduces undesirable numerical unstability
        if(static_cast<bool>(m_convexityModel))
        {
            m_convexityModel->accumulateConvexitySpreadGradient(model, multiplier, gradientBegin, gradientEnd, m_timeToExpiry);
        }
        */
    }

	void Futures::accumulateGradient(BaseModel const& baseModel,
                                     double multiplier,
									 GradientIterator gradientBegin,
                                     GradientIterator gradientEnd,
									 const CurveTypeConstPtr& curveType)
	{
		m_forwardRate->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);
	}
	
	void Futures::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void Futures::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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

    void Futures::update()
    {
        m_forwardRate->update();
    }    

    double Futures::getLastRelevantTime() const 
    {
        return m_forwardRate->getLastRelevantTime(); 
    }
	
    double Futures::getVariableInitialGuess(const double flowTime,
											const BaseModel* const model) const
	{
		LT_LOG << "initial guess for Fut" << getDescription().string()<< ", 3M" << endl; 
		
		// it can be worth calculating accrual period exactly
		// we should use the index spine curve discount factor, not the financial index discount factor
		return model->getVariableValueFromSpineDiscountFactor(flowTime, 
			model->getSpineDiscountFactor(flowTime - 0.25, CurveType::_3M()) / (1.0 + 0.25 * getRate()),
			CurveType::_3M());
	}

	double Futures::calculateRateDerivative(const BaseModelPtr&) const
	{
		return -1.0 ;
	}
	
	void Futures::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
		//	Do nothing
	}

	void Futures::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		//	Do nothing
	}

	double Futures::computeParRate(const BaseModelPtr& model)
	{
		return m_forwardRate->getValue(*model) + getConvexityAdjustment(model);
	}
	
	void Futures::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                               const BaseModel& model,
							   const double multiplier, 
							   IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		m_forwardRate->fillRepFlows(assetDomain, model, multiplier * m_forwardRate->getTenor( ) * m_contractSize, indexRepFlows);
	}

	double Futures::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
        const IDeA::DictionaryKey& instrumentKey = getKey<Futures>();
        LTQuant::GenericDataPtr instrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(instrumentListData, instrumentKey, instrumentData);

		if(!instrumentData)
		{
			LTQC_THROW( LTQC::DataQCException, "Cannot find futures table \"" << instrumentKey.getName().string() << "\"" );
		}

		double newPrice, newConvexity;
		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentData));
		
		for(size_t i(0); i < nbInstruments; ++i)
		{
			double convexityDiff = 0.0;
			if(compareAndTryGetNewRate(*instrumentData, i, IDeA_KEY(FUTURE, EXPIRY), IDeA_KEY(FUTURE, CONVEXITY), newConvexity))
			{
				convexityDiff =  getConvexityAdjustment() - newConvexity;
			}
			if(compareAndTryGetNewRate(*instrumentData, i, IDeA_KEY(FUTURE, EXPIRY), IDeA_KEY(FUTURE, PRICE), newPrice))
			{
				return (1.0 - getRate()) - newPrice + convexityDiff;
			}
		}
		
		LTQC_THROW( LTQC::DataQCException, "Cannot find a future with description \"" << getDescription().string() << "\" in table \"" << instrumentKey.getName().string() << "\"" );
	}

    ostream& Futures::print(ostream& out) const
    {
        out << Futures::getName() << getDescription().string();
        return out;
    }

    /**
        @brief Clone this futures instrument.

        Uses a lookup to maintain directed graph relationships.

        @param lookup A lookup of previuosly created clones.
        @return       The clone of this instance.
    */
    ICloneLookupPtr Futures::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new Futures(*this, lookup));
    }

    double Futures::getConvexityAdjustment(const BaseModelPtr& model) const
    {
        return (static_cast<bool>(m_convexityModel) ? m_convexityModel->computeConvexitySpread(model, m_timeToExpiry) : m_convexityAdjustment);
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    Futures::Futures(Futures const& original, CloneLookup& lookup) :
        CalibrationInstrument(original),
        m_forwardRate(lookup.get(original.m_forwardRate)),
        m_convexityAdjustment(original.m_convexityAdjustment),
        m_timeToExpiry(original.m_timeToExpiry),
        m_contractSize(original.m_contractSize)
    {
        if (m_convexityModel.get() != 0)
            LT_THROW_ERROR("Cannot clone FlexYCF that contains a futures convexity adjustment model");
    }
} // FlexYCF
