#include "stdafx.h"

//	FlexYCF
#include "CrossCurrencyFundingSwap.h"
#include "BaseModel.h"
#include "Data/GenericData.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "GlobalComponentCache.h"
#include "AllComponentsAndCaches.h"
#include "DataExtraction.h"
#include "FlexYcfUtils.h"
#include "DateUtils.h"
#include "RepFlowsData.h"
#include "StubUtils.h"
#include "FlexYCFCloneLookup.h"
#include "FXForward.h"
// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "MarketConvention.h"

using namespace LTQC;
using namespace std;
using namespace IDeA;

namespace
{
	FlexYCF::FloatingLegPtr createFloatingLeg(const LT::Str& maturity,
											  const LT::date valueDate,
											  const LT::date fixingDate,
											  const LT::date startDate,
											  const LT::date endDate,
											  const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
											  const FlexYCF::EndDateCalculationType applicableBackStubEndDateCalculationType,
											  FlexYCF::GlobalComponentCache* const globalComponentCache = 0)
	{
		using namespace FlexYCF;

		//	By default, do NOT adjust the calculation of the end date for the last period of the floating leg
		EndDateCalculationType backStubEndDateCalculationType(EndDateCalculationType::NotAdjusted);
		
		const LTQC::Tenor maturityTenor(maturity);
		const LTQC::Tenor domesticLegTenor(tradeDetails.m_floatFrequency.asTenorString());

		//	Checks if there is stub period. This works for domestic leg tenors of at least 1M
		if(maturityTenor < domesticLegTenor || maturityTenor.asMonths() % domesticLegTenor.asMonths() != 0)
		{
			backStubEndDateCalculationType = applicableBackStubEndDateCalculationType;
		}

		const FloatingLegArguments fltLegArgs(valueDate,
											  fixingDate,
											  startDate,
											  endDate,
											  tradeDetails.m_floatFrequency.asTenorString().string(),
											  tradeDetails.m_floatAccrualCalendar.string(),
											  tradeDetails.m_depRateMktConvention,
											  backStubEndDateCalculationType);

		return (globalComponentCache? globalComponentCache->get(fltLegArgs): FloatingLeg::create(fltLegArgs));
	}
    
    FlexYCF::FloatingLegPtr createForeignFloatingLeg(const LT::Str& maturity,
											  const LT::date valueDate,
											  const LT::date fixingDate,
											  const LT::date startDate,
											  const LT::date endDate,
											  const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
											  const FlexYCF::EndDateCalculationType applicableBackStubEndDateCalculationType,
											  FlexYCF::GlobalComponentCache* const globalComponentCache = 0)
	{
		using namespace FlexYCF;
        if( tradeDetails.m_forLegType == IDeA::CrossCcyForeignLegType::None)
        {
            return FlexYCF::FloatingLegPtr();
        }


		//	By default, do NOT adjust the calculation of the end date for the last period of the floating leg
		EndDateCalculationType backStubEndDateCalculationType(EndDateCalculationType::NotAdjusted);
		
		const LTQC::Tenor maturityTenor(maturity);
		const LTQC::Tenor forLegTenor(tradeDetails.m_forFloatFrequency.asTenorString());

		//	Checks if there is stub period. This works for domestic leg tenors of at least 1M
		if(maturityTenor < forLegTenor || maturityTenor.asMonths() % forLegTenor.asMonths() != 0)
		{
			backStubEndDateCalculationType = applicableBackStubEndDateCalculationType;
		}
        
        IDeA::DepositRateMktConvention depMktConv = tradeDetails.m_forDepRateMktConvention;
        depMktConv.m_currency = tradeDetails.m_forCurrency;
        depMktConv.m_index    = tradeDetails.m_forIndex;
        depMktConv.m_dcm      = tradeDetails.m_forFloatAccrualBasis;
       // depMktConv.m_rateTenor= forLegTenor;

		const FloatingLegArguments fltLegArgs(valueDate,
											  fixingDate,
											  startDate,
											  endDate,
											  forLegTenor.asTenorString().string(),
											  tradeDetails.m_forFloatAccrualCalendar.string(),
											  depMktConv,
											  backStubEndDateCalculationType);

		return (globalComponentCache? globalComponentCache->get(fltLegArgs): FloatingLeg::create(fltLegArgs));
	}
	template<class T>
	FlexYCF::EndDateCalculationType::Enum_t extractApplicableBackStubEndDateCalculationType(const T& curveParametersTable)
	{
		using namespace FlexYCF;

		//	Extract the applicable back stub end date calculation type:
		std::string endDateCalcTypeStr;
		IDeA::permissive_extract<std::string>(curveParametersTable,
											  IDeA_KEY(YC_CURVEPARAMETERS, CCYBASISBACKSTUBENDDATE),
											  endDateCalcTypeStr,
											  EndDateCalculationType::toString(CrossCurrencyFundingSwap::defaultBackStubEndDataCalculationType()).data());
		const EndDateCalculationType applicableBackStubEndDateCalculationType(EndDateCalculationType::fromString(endDateCalcTypeStr));
		return applicableBackStubEndDateCalculationType;
	}
}


namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<CrossCurrencyFundingSwap>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, CCYFUNDINGBASIS);
	}

    using namespace LTQuant;

    CrossCurrencyFundingSwap::CrossCurrencyFundingSwap(const string& description,
                                         const LT::date fixingDate,
                                         const LT::date startDate,
                                         const LT::date endDate,
                                         const double spread,
										 const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
										 const LT::date valueDate,
										 const EndDateCalculationType backStubEndDateCalculationType) :
		CalibrationInstrument(spread, getKeyName<CrossCurrencyFundingSwap>(), description, fixingDate, startDate, endDate),
        m_domesticLegTenor(ModuleDate::dateDescToYears(tradeDetails.m_floatFrequency.asTenorString().data())),
		m_floatingLeg(::createFloatingLeg(description, valueDate, fixingDate, startDate, endDate, tradeDetails, backStubEndDateCalculationType)),
		m_fixedLeg(FixedLeg::create(FixedLegArguments(valueDate,
                                                      fixingDate,
                                                      startDate,
                                                      endDate,
                                                      tradeDetails.m_floatFrequency.asTenorString().string(),
                                                      tradeDetails.m_depRateMktConvention.m_dcm.asString().data(),
													  tradeDetails.m_floatAccrualCalendar.string(),
                                                      tradeDetails.m_currency,tradeDetails.m_index))),
		m_firstDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate,startDate,tradeDetails.m_currency,tradeDetails.m_index))),
        m_lastDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate,endDate,tradeDetails.m_currency,tradeDetails.m_index))),
		m_details(tradeDetails),
        m_foreignLeg(::createForeignFloatingLeg(description, valueDate, fixingDate, startDate, endDate, tradeDetails, backStubEndDateCalculationType)),
        m_firstForeignDiscountFactor(DiscountFactor::create(DiscountFactorArguments( valueDate,startDate, tradeDetails.m_forCurrency,tradeDetails.m_forIndex))),
        m_lastForeignDiscountFactor(DiscountFactor::create(DiscountFactorArguments( valueDate, endDate, tradeDetails.m_forCurrency,tradeDetails.m_forIndex ))),
        m_isInitialized(false)
    {
    }
	
    CrossCurrencyFundingSwap::CrossCurrencyFundingSwap(const string& description,
                                         const LT::date fixingDate,
                                         const LT::date startDate,
                                         const LT::date endDate,
                                         const double spread,
										 const IDeA::CurrencyBasisSwapMktConvention& tradeDetails,
                                         const LT::date valueDate,
										 const EndDateCalculationType backStubEndDateCalculationType,
                                         GlobalComponentCache& globalComponentCache):
        CalibrationInstrument(spread, getKeyName<CrossCurrencyFundingSwap>(), description, fixingDate, startDate, endDate),
		m_domesticLegTenor(ModuleDate::dateDescToYears(tradeDetails.m_floatFrequency.asTenorString().data())),
		m_floatingLeg(::createFloatingLeg(description, valueDate, fixingDate, startDate, endDate, tradeDetails, backStubEndDateCalculationType, &globalComponentCache)),
		m_fixedLeg(globalComponentCache.get( FixedLeg::Arguments(valueDate,
																  fixingDate,
																  startDate,
																  endDate,
																  tradeDetails.m_floatFrequency.asTenorString().string(),
																  tradeDetails.m_depRateMktConvention.m_dcm.asString().data(),
																  tradeDetails.m_floatAccrualCalendar.string(),
                                                                  tradeDetails.m_currency,tradeDetails.m_index))),
        m_firstDiscountFactor(globalComponentCache.get( DiscountFactor::Arguments( valueDate,startDate,tradeDetails.m_currency,tradeDetails.m_index))),
        m_lastDiscountFactor(globalComponentCache.get( DiscountFactor::Arguments( valueDate, endDate,tradeDetails.m_currency,tradeDetails.m_index))),
		m_details(tradeDetails),
        m_foreignLeg(::createForeignFloatingLeg(description, valueDate, fixingDate, startDate, endDate, tradeDetails, backStubEndDateCalculationType, &globalComponentCache)),
        m_firstForeignDiscountFactor(globalComponentCache.get( DiscountFactor::Arguments( valueDate,startDate, tradeDetails.m_forCurrency,tradeDetails.m_forIndex))),
        m_lastForeignDiscountFactor(globalComponentCache.get( DiscountFactor::Arguments( valueDate, endDate, tradeDetails.m_forCurrency, tradeDetails.m_forIndex))),
        m_isInitialized(false)
    {
    }

	CalibrationInstrumentPtr CrossCurrencyFundingSwap::create(const LTQuant::GenericData& instrumentParametersTable, const LT::Ptr<LT::date>& buildDate, const LTQuant::GenericData& curveParametersTable, const LTQuant::GenericDataPtr&)
	{
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(curveParametersTable));		
		IDeA::CurrencyBasisSwapMktConvention& swapDetails = conventions->m_currencyBasisSwaps;

		// Try to extract the domestic tenor as specified in the instrument parameters table
		std::string domesticTenor;
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, CCYBASISSWAPDOMESTICTENOR), domesticTenor, swapDetails.m_floatFrequency.asTenorString().string());
		swapDetails.m_floatFrequency = LTQC::Tenor(domesticTenor);
		swapDetails.m_depRateMktConvention.m_rateTenor = LTQC::Tenor(0,1,0,0,0);
        swapDetails.m_depRateMktConvention.m_depositRateType = DepositRateType::FUNDING;

		std::string forCcy, forIndex, forLegType, forAccrualCalendar, forBasis, forFrequency;
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, CCYBASISSWAPLEG2CURRENCY), forCcy, swapDetails.m_forCurrency.string());
		IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, CCYBASISSWAPLEG2INDEX), forIndex, swapDetails.m_forIndex.string());
        IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, CCYBASISSWAPLEG2TYPE), forLegType, swapDetails.m_forLegType.asString().data());
	    IDeA::permissive_extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, CCYBASISSWAPLEG2FREQUENCY), forFrequency, swapDetails.m_forFloatFrequency.asString().data());
        
        swapDetails.m_forCurrency = forCcy;
		swapDetails.m_forIndex = forIndex;
        swapDetails.m_forLegType = IDeA::CrossCcyForeignLegType(forLegType);
	    swapDetails.m_forFloatFrequency = LTQC::Tenor(forFrequency);
        

		std::string maturity;
		LT::date fixingDate, actualBuildDate, startDate, endDate;
		setMaturityAndDates(instrumentParametersTable, buildDate, swapDetails.m_depRateMktConvention.m_fixingCalendar, swapDetails.m_depRateMktConvention.m_accrualValueCalendar, swapDetails.m_spotDays, maturity, actualBuildDate, fixingDate, startDate, endDate);
		
		//	Extract the applicable back stub end date calculation type:
		const EndDateCalculationType applicableBackStubEndDateCalculationType(extractApplicableBackStubEndDateCalculationType(curveParametersTable));

		return CalibrationInstrumentPtr(new CrossCurrencyFundingSwap(maturity, fixingDate, startDate, endDate, 0.0, swapDetails, actualBuildDate, applicableBackStubEndDateCalculationType));
	}

    void CrossCurrencyFundingSwap::createInstruments(CalibrationInstruments& instruments, 
                                              LTQuant::GenericDataPtr instrumentTable,
                                              LTQuant::GenericDataPtr masterTable,
                                              GlobalComponentCache& globalComponentCache,
                                              const LTQuant::PriceSupplierPtr)
    {
		const size_t nbCcySwaps(IDeA::numberOfRecords(*instrumentTable));

        // just finish if we have empty table or just headings
        if(nbCcySwaps == 0)
        {
            return;
        }

		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		const LT::date fixingDate(valueDate);

        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));		
		IDeA::CurrencyBasisSwapMktConvention swapDetails(conventions->m_currencyBasisSwaps);
		
		//	Extract the applicable back stub end date calculation type:
		const EndDateCalculationType applicableBackStubEndDateCalculationType(extractApplicableBackStubEndDateCalculationType(parametersTable));
		
        for(size_t cnt(0); cnt < nbCcySwaps; ++cnt)
        {
			 //	Description/Tenor (e.g. 5Y)
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, TENOR), cnt));

			if(!description.empty()) // skip empty rows
            {
				// get rate
                const double rate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, SPREAD), cnt));
                swapDetails.m_depRateMktConvention.m_rateTenor = LTQC::Tenor(0,1,0,0,0);
				swapDetails.m_depRateMktConvention.m_depositRateType = DepositRateType::FUNDING;
				
				std::string forCcy, forIndex, forLegType, forFrequency, index;
				IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, LEG2CURRENCY), cnt, forCcy, swapDetails.m_forCurrency.string());
				IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, LEG2INDEX), cnt, forIndex, swapDetails.m_forIndex.string());
                IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, LEG2TYPE), cnt, forLegType, swapDetails.m_forLegType.asString().data());
				IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, LEG2FREQUENCY), cnt, forFrequency, swapDetails.m_forFloatFrequency.asString().data());
                IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, LEG1INDEX), cnt, index, swapDetails.m_index.string());

                swapDetails.m_forCurrency = forCcy;
				swapDetails.m_forIndex = forIndex;
                swapDetails.m_forLegType = IDeA::CrossCcyForeignLegType(forLegType);
              
			    swapDetails.m_forFloatFrequency = LTQC::Tenor(forFrequency);
                swapDetails.m_index = index;

                // calc dates
                LT::date startDate, endDate;
                fillStartAndEndDates(instrumentTable, cnt, description, swapDetails.m_fixingCalendar.string(), swapDetails.m_floatAccrualCalendar.string(), swapDetails.m_spotDays, fixingDate, startDate, endDate);

				// build instrument
                CalibrationInstrumentPtr instrument( new CrossCurrencyFundingSwap( description,
                                                                            fixingDate,
                                                                            startDate,
                                                                            endDate,
                                                                            rate,
                                                                            swapDetails,
                                                                            valueDate,
																			applicableBackStubEndDateCalculationType,
                                                                            globalComponentCache)
                                                   );
                // add
				instruments.add(instrument);
            }
        }
    }
    
    void CrossCurrencyFundingSwap::updateInstruments(CalibrationInstruments& instrumentList, 
                                              LTQuant::GenericDataPtr instrumentTable, 
                                              size_t* instrumentIndex)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentTable));
        for(size_t i(0); i < nbInstruments; ++i)
        {
			//	Description/Tenor (e.g. 5Y)
			const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, TENOR), i));
            if(!description.empty())
            {
                const double rate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(CCYFUNDINGBASIS, SPREAD), i));
                instrumentList[*instrumentIndex]->setRate(rate);
                ++(*instrumentIndex);
            }
        }
    }

    const double CrossCurrencyFundingSwap::computeModelPrice(const BaseModelPtr model) const
    {
        double domPV = 0.0, forPV = 0.0;
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			domPV = m_floatingLeg->getValue(*model) + getRate() * m_fixedLeg->getValue(*model) + m_lastDiscountFactor->getValue(*model) - m_firstDiscountFactor->getValue(*model);
		}
		else
		{
			domPV = getRate() * m_fixedLeg->getValue(*model) + m_lastDiscountFactor->getValue(*model) - m_firstDiscountFactor->getValue(*model);
		}
        
        if(m_foreignLeg )
        {
            initializeForeignLegPricing(model);
            forPV = fxRate(model) * (m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel));
            return domPV - forPV;
        }
        return domPV;
    }
        
    void CrossCurrencyFundingSwap::accumulateGradient(BaseModel const& baseModel,
                                               double multiplier,
                                               GradientIterator gradientBegin,
                                               GradientIterator gradientEnd)
    {
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			// init the gradient argument with the gradient of the floating leg
			m_floatingLeg->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
		}

        // compute the fixed leg gradient, multiply it by the basis spread, and add it to the gradient argument
        m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd);

        // similarly, compute the gradient of the discount factors, subtract the "first" and add the "last" to the gradient argument
        m_firstDiscountFactor->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd);

        m_lastDiscountFactor->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
    }

	void CrossCurrencyFundingSwap::accumulateGradient(BaseModel const& baseModel,
											   double multiplier,
											   GradientIterator gradientBegin,
											   GradientIterator gradientEnd,
											   const CurveTypeConstPtr& curveType)
	{
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			// init the gradient argument with the gradient of the floating leg
			m_floatingLeg->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);
		}

        // compute the fixed leg gradient, multiply it by the basis spread, and add it to the gradient argument
        m_fixedLeg->accumulateGradient(baseModel, multiplier * getRate(), gradientBegin, gradientEnd, curveType);

        // similarly, compute the gradient of the discount factors, subtract the "first" and add the "last" to the gradient argument
        m_firstDiscountFactor->accumulateGradient(baseModel, -multiplier, gradientBegin, gradientEnd, curveType);

        m_lastDiscountFactor->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);	
	}

    void CrossCurrencyFundingSwap::update()
    {
        m_floatingLeg->update();
        m_fixedLeg->update();
        m_firstDiscountFactor->update();
        m_lastDiscountFactor->update();
        if( m_foreignLeg )
        {
            m_foreignLeg->update();
            m_firstForeignDiscountFactor->update();
            m_lastForeignDiscountFactor->update();
            if( m_fxSpotDomDiscountFactor )
                m_fxSpotDomDiscountFactor->update();
            if( m_fxSpotForDiscountFactor )
                m_fxSpotForDiscountFactor->update();
        }
    }

	double CrossCurrencyFundingSwap::getVariableInitialGuess(const double flowTime,
													  const BaseModel* const model) const
	{
		LT_LOG << "initial guess for XFunding" << getDescription().string()<< ", " << CurveType::Discount()->getDescription() << std::endl;
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			return model->getVariableValueFromSpineDiscountFactor(flowTime, pow(1.0 - getRate(), flowTime), CurveType::Discount());
		}
		else
		{
			return model->getVariableValueFromSpineDiscountFactor(flowTime, pow(1.0 + getRate(), flowTime), CurveType::Discount());
		}
	}

	double CrossCurrencyFundingSwap::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return m_fixedLeg->getValue(*model);
	}

	void CrossCurrencyFundingSwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
		m_fixedLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd);
	}

	void CrossCurrencyFundingSwap::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		m_fixedLeg->accumulateGradient(model, multiplier, gradientBegin, gradientEnd, curveType);
	}
	
	void CrossCurrencyFundingSwap::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void CrossCurrencyFundingSwap::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
	double CrossCurrencyFundingSwap::computeParRate(const BaseModelPtr& model)
	{
        double forPV = 0.0;
        if( m_foreignLeg )
        {
             initializeForeignLegPricing(model);
             forPV = fxRate(model) * (m_foreignLeg->getValue(*m_foreignModel) + m_lastForeignDiscountFactor->getValue(*m_foreignModel) - m_firstForeignDiscountFactor->getValue(*m_foreignModel));
        }

		if( domesticLegType() == IDeA::LegType::Floating )
		{
			return (m_firstDiscountFactor->getValue(*model) - m_lastDiscountFactor->getValue(*model) - m_floatingLeg->getValue(*model) + forPV) / m_fixedLeg->getValue(*model); 	
		}
		else
		{
			return (m_firstDiscountFactor->getValue(*model) - m_lastDiscountFactor->getValue(*model) + forPV) / m_fixedLeg->getValue(*model); 	
		}
	}

	void CrossCurrencyFundingSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                         const BaseModel& model,
							             const double multiplier, 
							             IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{	
		if( domesticLegType() == IDeA::LegType::Floating )
		{	
			m_floatingLeg->fillRepFlows(assetDomain, model, multiplier, fundingRepFlows);
		}
		
		m_fixedLeg->fillRepFlows(assetDomain, model, multiplier * getRate(), fundingRepFlows);
		//	"manually" add funding rep flows for last and first funding df:
        fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomain,m_lastDiscountFactor->getArguments().getPayDate()), multiplier);
		fundingRepFlows.addRepFlow(IDeA::Funding::Key(assetDomain,m_firstDiscountFactor->getArguments().getPayDate()), -multiplier);
	}

	void CrossCurrencyFundingSwap::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                         const BaseModel& model,
							             const double multiplier, 
							             IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			m_floatingLeg->fillRepFlows(assetDomain, model, multiplier, indexRepFlows);
		}
	}

	double CrossCurrencyFundingSwap::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<CrossCurrencyFundingSwap>(), IDeA_KEY(CCYFUNDINGBASIS, TENOR), IDeA_KEY(CCYFUNDINGBASIS, SPREAD));
	}

    ostream& CrossCurrencyFundingSwap::print(ostream& out) const
    {
        out << "XFUNDING" << getDescription().string();
        return out;
    }

   
    ICloneLookupPtr CrossCurrencyFundingSwap::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new CrossCurrencyFundingSwap(*this, lookup));
    }

    CrossCurrencyFundingSwap::CrossCurrencyFundingSwap(CrossCurrencyFundingSwap const& original, CloneLookup& lookup) : 
        CalibrationInstrument(original),
        m_domesticLegTenor(original.m_domesticLegTenor),
        m_details(original.m_details),
        m_floatingLeg(lookup.get(original.m_floatingLeg)),
        m_fixedLeg(lookup.get(original.m_fixedLeg)),
        m_firstDiscountFactor(lookup.get(original.m_firstDiscountFactor)),
        m_lastDiscountFactor(lookup.get(original.m_lastDiscountFactor)),
        m_foreignLeg(lookup.get(original.m_foreignLeg)),
        m_firstForeignDiscountFactor(lookup.get(original.m_firstForeignDiscountFactor)),
        m_lastForeignDiscountFactor(lookup.get(original.m_lastForeignDiscountFactor))
    {
    }

    void CrossCurrencyFundingSwap::fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const
	{
		cashFlowsTable.set<LT::date>("First Payment Date", 0, m_firstDiscountFactor->getArguments().getPayDate());
		cashFlowsTable.set<LT::date>("Last Payment Date", 0, m_lastDiscountFactor->getArguments().getPayDate());
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			cashFlowsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_floatingLeg->getCashFlows());
			//cashFlowsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_fixedLeg->getCashFlows());
		}
		else
		{
			cashFlowsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_fixedLeg->getCashFlows());	
		}

        if( m_foreignLeg )
        {
            cashFlowsTable.set<LT::date>("First Foreign Payment Date", 0, m_firstForeignDiscountFactor->getArguments().getPayDate());
		    cashFlowsTable.set<LT::date>("Last Foreign Payment Date", 0, m_lastForeignDiscountFactor->getArguments().getPayDate());
            cashFlowsTable.set<LTQuant::GenericDataPtr>("Foreign Leg", 0, m_foreignLeg->getCashFlows());
        }
	}

	void CrossCurrencyFundingSwap::fillCashFlowPVsTable(const BaseModel& model,
												 LTQuant::GenericData& cashFlowPVsTable) const
	{
		cashFlowPVsTable.set<LT::date>("First Payment Date", 0, m_firstDiscountFactor->getArguments().getPayDate());
		cashFlowPVsTable.set<double>("First Payment PV", 0, m_firstDiscountFactor->getValue(model));

		cashFlowPVsTable.set<LT::date>("Last Payment Date", 0, m_lastDiscountFactor->getArguments().getPayDate());
		cashFlowPVsTable.set<double>("Last Payment PV", 0, m_lastDiscountFactor->getValue(model));
		if( domesticLegType() == IDeA::LegType::Floating )
		{
			cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_floatingLeg->computeCashFlowPVs(model));
			// cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Spreads", 0, m_fixedLeg->computeCashFlowPVs(model));
		}
		else
		{
			cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Domestic Leg", 0, m_fixedLeg->computeCashFlowPVs(model));
		}
         
        if( m_foreignLeg )
        {
           initializeForeignLegPricing(model);
           cashFlowPVsTable.set<LT::date>("First Foreign Payment Date", 0, m_firstForeignDiscountFactor->getArguments().getPayDate());
		   cashFlowPVsTable.set<double>("First Foreign Payment PV", 0, m_firstForeignDiscountFactor->getValue(*m_foreignModel));

		   cashFlowPVsTable.set<LT::date>("Last Foreign Payment Date", 0, m_lastForeignDiscountFactor->getArguments().getPayDate());
	 	   cashFlowPVsTable.set<double>("Last Foreign Payment PV", 0, m_lastForeignDiscountFactor->getValue(*m_foreignModel));
           cashFlowPVsTable.set<LTQuant::GenericDataPtr>("Foreign Leg", 0, m_foreignLeg->computeCashFlowPVs(*m_foreignModel));
        }

	}

     void CrossCurrencyFundingSwap::finishCalibration(const BaseModelPtr model)
     {
        CalibrationInstrument::finishCalibration(model);
        m_floatingLeg->cleanupCashFlows();
        m_fixedLeg->cleanupCashFlows();
        m_firstDiscountFactor.reset();
        m_lastDiscountFactor.reset();
        if(m_foreignLeg)
        {
            m_foreignLeg->cleanupCashFlows();
            m_firstForeignDiscountFactor.reset();
            m_lastForeignDiscountFactor.reset();
            
            m_fxSpotDomDiscountFactor.reset();
            m_fxSpotForDiscountFactor.reset();
        }
     }

     void CrossCurrencyFundingSwap::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
     {
         const CrossCurrencyFundingSwapPtr ourTypeSrc=std::tr1::static_pointer_cast<CrossCurrencyFundingSwap>(src);
         if(!ourTypeSrc)
         {
             LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
         }

         m_firstDiscountFactor=ourTypeSrc->m_firstDiscountFactor;
         m_lastDiscountFactor=ourTypeSrc->m_lastDiscountFactor;

         CalibrationInstrument::reloadInternalState(src);
     }

     void CrossCurrencyFundingSwap::initializeForeignLegPricing(const BaseModelPtr model) const
        {
            if( !m_isInitialized )
            {
                LT::Str market; 
                for (size_t i = 1; i < model->getDependentMarketData()->table->rowsGet(); ++i) 
                {
                    AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(model->getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                    LT::Str asset = IDeA::extract<LT::Str>(model->getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
                    if (adType == IDeA::AssetDomainType::IR )
                    {
                        if( asset.compareCaseless(m_details.m_forCurrency) == 0 )
                        {
                            market = IDeA::extract<LT::Str>(model->getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
                            m_foreignCcy = asset;
                            break;
                        }
                    }
                }
                
                if(model->hasDependentModel(IRAssetDomain(m_foreignCcy, market)))
                {
                    m_foreignModel = model->getDependentModel(IRAssetDomain(m_foreignCcy, market));
                }
                else
                {
                    LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model for " << m_details.m_forCurrency.data());
                }
               
                if( m_foreignCcy.compareCaseless(m_details.m_currency) != 0 )
                {
                    m_fxSpot = model->getDependentFXRate(FXSpotAssetDomain(m_foreignCcy, m_details.m_currency));
              
                    LT::date valueDate = model->getValueDate();
                    std::string fxPair = m_foreignCcy.string() + m_details.m_currency.string();
                    LT::date fxSpot = LTQuant::getFXSpotDate(valueDate, fxPair);
                    m_fxSpotDomDiscountFactor = DiscountFactor::create(DiscountFactorArguments(valueDate, fxSpot,m_details.m_currency));
                    m_fxSpotForDiscountFactor = DiscountFactor::create(DiscountFactorArguments(valueDate, fxSpot,m_foreignCcy));
                }
                else
                {
                    m_fxSpot = 1.0;
                    LT::date valueDate = model->getValueDate();
                   
                    m_fxSpotDomDiscountFactor = DiscountFactor::create(DiscountFactorArguments(valueDate, valueDate,m_details.m_currency));
                    m_fxSpotForDiscountFactor = DiscountFactor::create(DiscountFactorArguments(valueDate, valueDate,m_foreignCcy));
                }
                m_isInitialized = true;
            }
        }
         
        void CrossCurrencyFundingSwap::initializeForeignLegPricing(const BaseModel& model) const
        {
            if( !m_isInitialized )
            {
                LT::Str market; 
                for (size_t i = 1; i < model.getDependentMarketData()->table->rowsGet(); ++i) 
                {
                    AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
                    LT::Str asset = IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
                    if (adType == IDeA::AssetDomainType::IR )
                    {
                        if( asset.compareCaseless(m_details.m_forCurrency) == 0 )
                        {
                            market = IDeA::extract<LT::Str>(model.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
                            m_foreignCcy = asset;
                            break;
                        }
                    }
                }
                
                if(model.hasDependentModel(IRAssetDomain(m_foreignCcy, market)))
                {
                    m_foreignModel = model.getDependentModel(IRAssetDomain(m_foreignCcy, market));
                }
                else
                {
                    LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model for " << m_details.m_forCurrency.data());
                }
                m_isInitialized = true;
            }
        }
}   // FlexYCF