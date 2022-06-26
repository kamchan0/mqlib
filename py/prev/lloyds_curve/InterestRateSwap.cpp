/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "InterestRateSwap.h"
#include "BaseModel.h"
#include "Data/GenericData.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "ModuleDate/InternalInterface/ScheduleGeneratorFactory.h"
#include "AllComponentsAndCaches.h"
#include "GlobalComponentCache.h"
#include "TenorUtils.h"
#include "DataExtraction.h"
#include "FlexYcfUtils.h"
#include "FlexYCFCloneLookup.h"

// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "MarketConvention.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<InterestRateSwap>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, SWAPS);
	}

    using namespace LTQuant;
    
	InterestRateSwap::InterestRateSwap(const string& description,
									   const LT::date fixingDate,
									   const LT::date startDate,
									   const LT::date endDate,
									   const double swapRate,
									   const IDeA::SwapMktConvention& tradeDetails,
									   const LT::date valueDate):					 
		CalibrationInstrument(swapRate, getKeyName<InterestRateSwap>(), description, fixingDate, startDate, endDate),
		m_fixedLeg(FixedLeg::create(FixedLeg::Arguments(valueDate,
														fixingDate,
                                                        startDate,
                                                        endDate,
                                                        tradeDetails.m_fixedFrequency.asTenorString().string(),
                                                        tradeDetails.m_fixedAccrualBasis.asString().data(),
														tradeDetails.m_fixedAccrualCalendar.string(),
														"",
														"",
														tradeDetails.m_depRateMktConvention.m_rollConvention,
														tradeDetails.m_depRateMktConvention.m_rollRuleConvention))),
		m_floatingLeg(FloatingLeg::create(FloatingLeg::Arguments(valueDate,
																 fixingDate,
                                                                 startDate,
                                                                 endDate,
                                                                 tradeDetails.m_floatFrequency.asTenorString().string(),
																 tradeDetails.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_depRateMktConvention))),
        m_fixedLegTenor(tenorDescToYears(tradeDetails.m_fixedFrequency.asTenorString().string())),
        m_floatingLegTenor(tenorDescToYears(tradeDetails.m_floatFrequency.asTenorString().string())),
		m_details(tradeDetails)
    {
    }

    InterestRateSwap::InterestRateSwap(const string& description,
									   const LT::date fixingDate,
									   const LT::date startDate,
									   const LT::date endDate,
									   const double swapRate,
									   const IDeA::SwapMktConvention& tradeDetails,
									   const LT::date valueDate,
                                       GlobalComponentCache& globalComponentCache):
		CalibrationInstrument(swapRate, getKeyName<InterestRateSwap>(), description, fixingDate, startDate, endDate),
		m_fixedLeg(globalComponentCache.get( FixedLeg::Arguments(valueDate,
														fixingDate,
                                                        startDate,
                                                        endDate,
                                                        tradeDetails.m_fixedFrequency.asTenorString().string(),
                                                        tradeDetails.m_fixedAccrualBasis.asString().data(),
														tradeDetails.m_fixedAccrualCalendar.string()))),

		m_floatingLeg(globalComponentCache.get( FloatingLeg::Arguments(valueDate,
																 fixingDate,
                                                                 startDate,
                                                                 endDate,
                                                                 tradeDetails.m_floatFrequency.asTenorString().string(),
 																 tradeDetails.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_depRateMktConvention))),
        m_fixedLegTenor(tenorDescToYears(tradeDetails.m_fixedFrequency.asTenorString().string())),
        m_floatingLegTenor(tenorDescToYears(tradeDetails.m_floatFrequency.asTenorString().string())),
		m_details(tradeDetails)
    {
    }

    InterestRateSwap::InterestRateSwap(const string& description,
									   const LT::date fixingDate,
									   const LT::date startDate,
									   const LT::date endDate,
									   const double swapRate,
									   const IDeA::SwapMktConvention& tradeDetails,
									   const LT::date valueDate,
                                       const double liborFixing,
                                       GlobalComponentCache& globalComponentCache):
		CalibrationInstrument(swapRate, getKeyName<InterestRateSwap>(), description, fixingDate, startDate, endDate),
		m_fixedLeg(globalComponentCache.get( FixedLeg::Arguments(valueDate,
														fixingDate,
                                                        startDate,
                                                        endDate,
                                                        tradeDetails.m_fixedFrequency.asTenorString().string(),
                                                        tradeDetails.m_fixedAccrualBasis.asString().data(),
														tradeDetails.m_fixedAccrualCalendar.string()))),

		m_floatingLeg(globalComponentCache.get( FloatingLeg::Arguments(valueDate,
																 fixingDate,
                                                                 startDate,
                                                                 endDate,
                                                                 tradeDetails.m_floatFrequency.asTenorString().string(),
																 tradeDetails.m_floatAccrualCalendar.string(),
                                                                 tradeDetails.m_depRateMktConvention, liborFixing))),
        m_fixedLegTenor(tenorDescToYears(tradeDetails.m_fixedFrequency.asTenorString().string())),
        m_floatingLegTenor(tenorDescToYears(tradeDetails.m_floatFrequency.asTenorString().string())),
		m_details(tradeDetails)
    {
    }

	CalibrationInstrumentPtr InterestRateSwap::create(const LTQuant::GenericData& instrumentParametersTable, 
													  const LT::Ptr<LT::date>& buildDate,
													  const LTQuant::GenericData& curveParametersTable,
													  const LTQuant::GenericDataPtr&)
	{
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(curveParametersTable));				
		IDeA::SwapMktConvention& swapDetails = conventions->m_swaps;

		std::string swap_floatTenorStr, swap_fixedTenorStr, swap_fixedBasisStr, swapRollRule;
		
		// initialize from curve parameters
		{
			std::string floatTenorStr, fixedTenorStr, fixedBasisStr, rollRuleStr;
			IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FIXEDTENOR), fixedTenorStr, swapDetails.m_fixedFrequency.asTenorString().string());
			IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FLOATTENOR), floatTenorStr, swapDetails.m_floatFrequency.asTenorString().string());
			IDeA::permissive_extract<std::string>(curveParametersTable, IDeA_KEY(YC_CURVEPARAMETERS, FIXEDBASIS), fixedBasisStr, swapDetails.m_fixedAccrualBasis.asString().data());

			//	try extract from instrument parameters
			IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FIXEDTENOR), swap_fixedTenorStr, fixedTenorStr);
			IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FLOATTENOR), swap_floatTenorStr, floatTenorStr);
			IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, FIXEDBASIS), swap_fixedBasisStr, fixedBasisStr);
			if( IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ROLLRULEMETHOD), swapRollRule, rollRuleStr) )
			{
				swapDetails.m_depRateMktConvention.m_rollRuleConvention = LTQC::RollRuleMethod(swapRollRule);
			}
		}

		// override with parameters
		swapDetails.m_floatFrequency = LTQC::Tenor(swap_floatTenorStr.data());
		swapDetails.m_depRateMktConvention.m_rateTenor = swapDetails.m_floatFrequency;
		swapDetails.m_fixedFrequency = LTQC::Tenor(swap_fixedTenorStr.data());
		swapDetails.m_fixedAccrualBasis = LTQC::DayCountMethod(swap_fixedBasisStr.data());
		
		std::string maturity;
		LT::date fixingDate, actualBuildDate, startDate, endDate;
		setMaturityAndDates(instrumentParametersTable, buildDate, swapDetails.m_depRateMktConvention.m_fixingCalendar, swapDetails.m_depRateMktConvention.m_accrualValueCalendar, swapDetails.m_spotDays, maturity, actualBuildDate, fixingDate, startDate, endDate);

		return CalibrationInstrumentPtr(new InterestRateSwap(maturity,
														     fixingDate,
														     startDate,
														     endDate,
														     0.0,
															 swapDetails,
														     actualBuildDate));
	}

    void InterestRateSwap::createInstruments(CalibrationInstruments& instruments,
                                             LTQuant::GenericDataPtr instrumentTable,
                                             LTQuant::GenericDataPtr masterTable,
                                             GlobalComponentCache& globalComponentCache,
                                             const LTQuant::PriceSupplierPtr)
    {
		const size_t nbVanillaSwaps(IDeA::numberOfRecords(*instrumentTable));

        // just finish if we have empty table or just headings
        if(nbVanillaSwaps == 0)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		const LT::date fixingDate(valueDate);

        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		if (!parametersTable)
			LTQC_THROW(IDeA::MarketException, "Null parameters table");

		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));				
		IDeA::SwapMktConvention& tradeDetails = conventions->m_swaps;
		
		//	Model
		// Swap Fixing
        for(size_t cnt(0); cnt < nbVanillaSwaps; ++cnt)
        {
			const std::string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(SWAP, TENOR), cnt));
            
            if(!description.empty())
            {
                const double swapRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(SWAP, RATE), cnt));

                LT::date startDate, endDate;
                fillStartAndEndDates(instrumentTable, cnt, description, tradeDetails.m_fixingCalendar.data(), tradeDetails.m_fixedAccrualCalendar.data(), tradeDetails.m_spotDays, fixingDate, startDate, endDate);

                string fixedTenor, floatingTenor, fixedBasisName;
                
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(SWAP, FIXEDTENOR), cnt, fixedTenor, tradeDetails.m_fixedFrequency.asTenorString().string());
                IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(SWAP, FLOATTENOR), cnt, floatingTenor, tradeDetails.m_floatFrequency.asTenorString().string()); 
				IDeA::permissive_extract<string>(instrumentTable, IDeA_KEY(SWAP, FIXEDBASIS), cnt, fixedBasisName, tradeDetails.m_fixedAccrualBasis.asString().data());

                // Check Libor Fixings table to look for the libor with 
                //  the right tenor and pass it to the swap so that is 
                //  used instead of the forward rate for the 1st payment
                //  date of the floating leg.
                string liborFixingDesc("");
                double liborFixing(0.);

                // Retrieve Libor Fixings table                            
                if(masterTable->doesTagExist("Market Data"))
                {
                    LTQuant::GenericDataPtr marketDataTable;
                    masterTable->permissive_get<LTQuant::GenericDataPtr>("Market Data", 0, marketDataTable);

                    if(static_cast<bool>(marketDataTable) && marketDataTable->doesTagExist("Libor Fixings"))
                    {
                        LTQuant::GenericDataPtr liborFixingsTable;
                        
                        marketDataTable->permissive_get<LTQuant::GenericDataPtr>("Libor Fixings", 0, liborFixingsTable);
                        
                        if(static_cast<bool>(liborFixingsTable))
                        {
							LT_LOG << "Using LIBOR Fixings" << std::endl;
							// Iterate through the Libor fixings and check if the Libor with the right tenor is provided
                            for(size_t liborFixingCount(0); liborFixingCount < liborFixingsTable->numItems() - 1; ++liborFixingCount)
                            {
                                liborFixingsTable->permissive_get<string>("Description", liborFixingCount, liborFixingDesc, "");
                                if(liborFixingDesc == floatingTenor)
                                {
                                    liborFixing = liborFixingsTable->get<double>("Rate", liborFixingCount);
                                    break;
                                }
                            }
                        }
						else
						{
							LT_LOG << "NOT Using LIBOR Fixings" << std::endl;
						}
                    }
                }
                
				tradeDetails.m_fixedFrequency = LTQC::Tenor(fixedTenor);
				tradeDetails.m_fixedAccrualBasis = LTQC::DayCountMethod(fixedBasisName);
				tradeDetails.m_floatFrequency = LTQC::Tenor(floatingTenor);
				tradeDetails.m_depRateMktConvention.m_rateTenor = tradeDetails.m_floatFrequency;


                // create the IRS passing the LIBOR fixing rate if provided
                CalibrationInstrumentPtr instrument(liborFixingDesc == floatingTenor?    
                                                    new InterestRateSwap(description, fixingDate, startDate, endDate, swapRate, tradeDetails,
																		 valueDate, liborFixing, globalComponentCache) :
                                                    new InterestRateSwap(description, fixingDate, startDate, endDate, swapRate, tradeDetails,
																		 valueDate, globalComponentCache) 
                                                    );
                instruments.add(instrument);
            }
        }
    }

    void InterestRateSwap::updateInstruments(CalibrationInstruments& instrumentList, 
                                              LTQuant::GenericDataPtr instrumentTable, 
                                              size_t* instrumentIndex)
    {
		const size_t nbVanillaSwaps(IDeA::numberOfRecords(*instrumentTable));

        // just finish if we have empty table or just headings
        if(nbVanillaSwaps == 0)
        {
            return;
        }

        for(size_t i(0); i < nbVanillaSwaps; ++i)
        {
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(SWAP, TENOR), i));
            if(!description.empty())
            {
                const double swapRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(SWAP, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(swapRate);
                ++(*instrumentIndex);
            }
        }
    }
    
    void InterestRateSwap::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);

        m_floatingLeg->cleanupCashFlows();
        m_fixedLeg->cleanupCashFlows();
    }

    void InterestRateSwap::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
        const InterestRateSwapPtr ourTypeSrc=std::tr1::static_pointer_cast<InterestRateSwap>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        CalibrationInstrument::reloadInternalState(src);
    }

    const double InterestRateSwap::computeModelPrice(const BaseModelPtr model) const
    {
        return m_fixedLeg->getValue(*model) * getRate() - m_floatingLeg->getValue(*model);
    }
        
    void InterestRateSwap::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {
        // compute the fixed leg gradient and multiply it by the swap rate
        m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd);
        // subtract the floating leg gradient
        m_floatingLeg->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd);
    }

	void InterestRateSwap::accumulateGradient(BaseModel const& baseModel,
											 double multiplier,
											 GradientIterator gradientBegin,
											 GradientIterator gradientEnd,
											 const CurveTypeConstPtr& curveType)
	{
		// compute the fixed leg gradient and multiply it by the swap rate
        m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd, curveType);
        // subtract the floating leg gradient
        m_floatingLeg->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd, curveType);
	}
	
	void InterestRateSwap::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
	
	void InterestRateSwap::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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

    void InterestRateSwap::update()
    {
        m_floatingLeg->update();
        m_fixedLeg->update();
    }

    double InterestRateSwap::getLastRelevantTime() const 
    {
        return ModuleDate::getYearsBetween(m_fixedLeg->getValueDate(), getEndDate()); 
    }
    
	double InterestRateSwap::getVariableInitialGuess(const double flowTime,
													 const BaseModel* const model) const
	{
		LT_LOG << "initial guess for S" << getDescription().string()<< ", " 
			<< CurveType::_3M()->getDescription() << " (fxd tenor: " << m_fixedLegTenor << ")" << endl;
		return model->getVariableValueFromSpineDiscountFactor(flowTime, 
															  1.0 / pow(1.0 + getRate(), flowTime ),
															  CurveType::_3M());	// This depends on which curve the kpp places swaps!
														 // CurveType::getFromYearFraction(m_floatingLegTenor));
	}

	double InterestRateSwap::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return m_fixedLeg->getValue(*model);
	}

	void InterestRateSwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
		m_fixedLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd);
	}

	void InterestRateSwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		m_fixedLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd, curveType);
	}


	double InterestRateSwap::computeParRate(const BaseModelPtr& model)
	{
		return m_floatingLeg->getValue(*model) / m_fixedLeg->getValue(*model);
	}
	
	void InterestRateSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                        const BaseModel& model,
										const double multiplier, 
										IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
		m_fixedLeg->fillRepFlows(assetDomain, model, multiplier * getRate(), fundingRepFlows);
		m_floatingLeg->fillRepFlows(assetDomain, model, -multiplier, fundingRepFlows);
	}

	void InterestRateSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                        const BaseModel& model,
										const double multiplier, 
										IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		//	Note: no index rep flows for fixed leg
		m_floatingLeg->fillRepFlows(assetDomain, model, -multiplier, indexRepFlows);
	}

	double InterestRateSwap::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<InterestRateSwap>(), IDeA_KEY(SWAP, TENOR), IDeA_KEY(SWAP, RATE));
	}

    ostream& InterestRateSwap::print(ostream& out) const
    {
        out << "S" << getDescription().string();
        return out;
    }

    /**
        @brief Clone this interest rate swap.

        Uses a lookup to maintain directed graph relationships.

        @param lookup A lookup of previuosly created clones.
        @return       The clone of this instance.
    */
    ICloneLookupPtr InterestRateSwap::cloneWithLookup(CloneLookup& lookup) const
    {
        return CalibrationInstrumentPtr(new InterestRateSwap(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    InterestRateSwap::InterestRateSwap(InterestRateSwap const& original, CloneLookup& lookup) : 
        CalibrationInstrument(original),
        m_details(original.m_details),
        m_fixedLegTenor(original.m_fixedLegTenor),
        m_floatingLegTenor(original.m_floatingLegTenor),
        m_fixedLeg(lookup.get(original.m_fixedLeg)),
        m_floatingLeg(lookup.get(original.m_floatingLeg))
    {
    }

	void InterestRateSwap::fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const
	{
		cashFlowsTable.set<LTQuant::GenericDataPtr>("Fixed Leg", 0, m_fixedLeg->getCashFlows());
		cashFlowsTable.set<LTQuant::GenericDataPtr>("Floating Leg", 0, m_floatingLeg->getCashFlows());
	}

	void InterestRateSwap::fillCashFlowPVsTable(const BaseModel& model,
												LTQuant::GenericData& cashFlowPVsTable) const
	{
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Fixed Leg", 0, m_fixedLeg->computeCashFlowPVs(model));
		cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Floating Leg", 0, m_floatingLeg->computeCashFlowPVs(model));
	}
}   // FlexYCF