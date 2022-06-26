/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

// FlexYCF
#include "CashInstrument.h"
#include "CalibrationInstruments.h"
#include "BaseModel.h"
#include "ForwardRate.h"
#include "GlobalComponentCache.h"
#include "TenorUtils.h"
#include "DataExtraction.h"
#include "FlexYcfUtils.h"
#include "FlexYCFCloneLookup.h"

// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"
#include "MarketConvention.h"
#include "TradeConventionCalcH.h"

// QuantCore
#include "DayCount.h"
#include "Tenor.h"

// LTQuantCore
#include "dates\DateBuilder.h"

// LTQuantLib
#include "Data/GenericData.h"
#include "ModuleDate/InternalInterface/DayCounter.h"
#include "DateUtils.h"


using namespace std;
using namespace LTQC;
using namespace IDeA;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<CashInstrument>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, CASH);
	}

    using namespace LTQuant;
    using namespace ModuleDate;

    CashInstrument::CashInstrument(const string& description, 
                                   const LT::date fixingDate,
                                   const LT::date startDate,
                                   const LT::date endDate,
                                   const double rate,
                                   const DayCounterConstPtr basis,
                                   const LT::date valueDate,
                                   GlobalComponentCache& globalComponentCache) :
        CalibrationInstrument(rate, getKeyName<CashInstrument>(), description, fixingDate, startDate, endDate),
		m_forwardRate(globalComponentCache.get( ForwardRate::Arguments( valueDate, 
                                                                        fixingDate, 
                                                                        startDate, 
                                                                        endDate, 
                                                                        description, 
                                                                        basis,
                                                                        globalComponentCache)))
    {
    }

	CashInstrument::CashInstrument(const std::string& tenorDescription,
								   const LT::date fixingDate,
                                   const LT::date startDate,
                                   const LT::date endDate,
                                   const double rate,
                                   const DayCounterConstPtr basis,
                                   const LT::date valueDate) :
        CalibrationInstrument(rate, getKeyName<CashInstrument>(), tenorDescription, fixingDate, startDate, endDate),
		m_forwardRate(new ForwardRate(ForwardRate::Arguments( valueDate, 
                                             fixingDate, 
                                             startDate, 
                                             endDate, 
                                             tenorDescription, 
                                             basis)))
    {
    }
    
	CashInstrument::~CashInstrument()
    {
    }

	CalibrationInstrumentPtr CashInstrument::create(const LTQuant::GenericData& instrumentParametersTable,
												    const LT::Ptr<LT::date>& buildDate,
													const LTQuant::GenericData& curveParametersTable,
													const LTQuant::GenericDataPtr&)
	{
		
		LT::date startDate, fixingDate, endDate;
		const IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(curveParametersTable));
		const IDeA::DepositRateMktConvention& tradeDetails(conventions->m_depo);
		const DayCounterPtr basis(LTQC::DayCount::create(tradeDetails.m_dcm));
        
		if(buildDate)
		{
			fixingDate = *buildDate;
			const bool found(IDeA::permissive_extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE), fixingDate, *buildDate));
			if (!found)
				startDate = IDeA::TradeConventionCalcH::getSpotDate(Date(*buildDate), conventions->m_depo.m_fixingCalendar, conventions->m_depo.m_accrualValueCalendar, conventions->m_depo.m_spotDays).getAsLTdate();
		}
		else
		{
			startDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE));
			fixingDate = IDeA::IRDepositRateCalcH::calculateFixingDate(Date(startDate), conventions->m_depo).getAsLTdate();
		}
		endDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE));

		return CalibrationInstrumentPtr(new CashInstrument(tradeDetails.m_rateTenor.asString().string(),
															fixingDate,
															startDate,
															endDate, 
															0.0, 
															basis,
															*buildDate));
	}

    void CashInstrument::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_forwardRate.reset();
    }

    void CashInstrument::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
        const CashInstrumentPtr ourTypeSrc=std::tr1::static_pointer_cast<CashInstrument>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        m_forwardRate=ourTypeSrc->m_forwardRate;

        CalibrationInstrument::reloadInternalState(src);
    }

    void CashInstrument::createInstruments(CalibrationInstruments& instrumentList, 
                                           LTQuant::GenericDataPtr instrumentTable,
                                           LTQuant::GenericDataPtr masterTable,
                                           GlobalComponentCache& globalComponentCache,
                                           const LTQuant::PriceSupplierPtr)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		const LT::date fixingDate(valueDate);

		//	Get the deposit rate market conventions
        const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		const IDeA::IRCurveMktConventionPtr conventions(createIRCurveMktConventions(parametersTable));
		const IDeA::DepositRateMktConvention& tradeDetails(conventions->m_depo);

		//	Get the basis
		const DayCounterPtr basis(LTQC::DayCount::create(tradeDetails.m_dcm));

		for(size_t i(0); i < IDeA::numberOfRecords(*instrumentTable); ++i)
        {
			std::string emptyStr, description;
			LT::date startDate, endDate;
			//	Description/Tenor (e.g.: 3M)
			bool foundTenor		= IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CASH, TENOR), i, description, emptyStr);	
			bool foundEndDate	= IDeA::permissive_extract<LT::date>(	*instrumentTable, IDeA_KEY(CASH, ENDDATE), i, endDate, LT::date() );
			bool foundStartDate = IDeA::permissive_extract<LT::date >(	*instrumentTable, IDeA_KEY(CASH, STARTDATE), i, startDate, LT::date() );

			if(!description.empty() || (foundEndDate && foundStartDate))
            {
                const double cashRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(CASH, RATE), i));
				bool lessThan1M =  true;
                
                LTQC::Tenor spotDays = tradeDetails.m_spotDays;
                std::string shortTenor;

                if( description.compare("O/N") == 0 )
                {
                    description = "O/N";
                    shortTenor = "1D";
                    spotDays = LTQC::Tenor("0D");
                } 
                else if( description.compare("1D") == 0)
                {
                    description = "1D";
                    shortTenor = "1D";
                    spotDays = LTQC::Tenor("0D");
                }
                else if( description.compare("2D") == 0)
                {
                    if( IDeA::TradeConventionCalcH::getSpotDate(Date(fixingDate), tradeDetails.m_fixingCalendar.string(), tradeDetails.m_accrualValueCalendar.string(), spotDays).getAsLTdate() > fixingDate )
                    {
                        description = "T/N";
                        shortTenor = "1D";
                        spotDays = LTQC::Tenor("1D");
                    }
                    else
                    {
                        description = "2D";
                        shortTenor = "2D";
                        spotDays = LTQC::Tenor("0D");
                    }
                } 
                else if ( description.compare("T/N") == 0 )
                {
                     description = "T/N";
                     shortTenor = "1D";
                     spotDays = LTQC::Tenor("1D");
                }
				else
					lessThan1M = LTQC::Tenor(description).asDays() < LTQC::Tenor("1M").asDays();

				if (!foundStartDate)
				{
					std::string spotDaysStr, spotRollConvStr, fixingCalendar, accrualValueCalendar;
					LTQC::RollConvMethod spotRollConv;
					LTQC::Tenor spotDaysTenor;

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CASH, SPOTDAYS), i, spotDaysStr, emptyStr);
					spotDaysTenor = (spotDaysStr != emptyStr) ? LTQC::Tenor(spotDaysStr) : spotDays;

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CASH, FIXINGCALENDAR), i, fixingCalendar, emptyStr);
					fixingCalendar = (fixingCalendar != emptyStr) ? fixingCalendar : tradeDetails.m_fixingCalendar.string();

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CASH, ACCRUALCALENDAR), i, accrualValueCalendar, emptyStr);
					accrualValueCalendar = (accrualValueCalendar != emptyStr) ? accrualValueCalendar : tradeDetails.m_accrualValueCalendar.string();

					// spot roll by default is Following
					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CASH, SPOTROLLCONVENTION), i, spotRollConvStr, emptyStr);
					spotRollConv = (spotRollConvStr != emptyStr) ? LTQC::RollConvMethod(spotRollConvStr) : LTQC::RollConvMethod(RollConvMethod::Following);

					startDate = IDeA::TradeConventionCalcH::getSpotDate(Date(valueDate), fixingCalendar, accrualValueCalendar, spotDaysTenor, spotRollConv).getAsLTdate();
				}

				if(!foundEndDate)
				{
					std::string rollConv, rollRuleConv, accrualValueCalendar;

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CASH, ACCRUALCALENDAR), i, accrualValueCalendar, emptyStr);
					accrualValueCalendar = (accrualValueCalendar != emptyStr) ? accrualValueCalendar : tradeDetails.m_accrualValueCalendar.string();

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CASH, ROLLCONVENTION), i, rollConv, emptyStr);
					rollConv =	( rollConv != emptyStr )	
								? rollConv 
								: (!lessThan1M	? LTQC::RollConvMethod(RollConvMethod::ModifiedFollowing).asString().data()
												: LTQC::RollConvMethod(RollConvMethod::Following).asString().data() );

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(CASH, ROLLRULECONVENTION), i, rollRuleConv, emptyStr);
					rollRuleConv = (rollRuleConv != emptyStr) 
								?	rollRuleConv 
								: (!lessThan1M	? LTQC::RollRuleMethod(RollRuleMethod::BusinessEOM).asString().data()
												: LTQC::RollRuleMethod(RollRuleMethod::None).asString().data() );

					endDate = LTQC::DateBuilder::dateAdd(	startDate, 
															shortTenor.empty() ? description : shortTenor, 
															accrualValueCalendar, 
															rollConv, 
															rollRuleConv, 
															LTQC::RollConvMethod(RollConvMethod::None).asString().data()).getAsLTdate();
				}              

                CalibrationInstrumentPtr instrument( new CashInstrument( foundTenor ? description : endDate.toDDMMMYYYYString(),
																		 fixingDate,
                                                                         startDate,
                                                                         endDate,
                                                                         cashRate,
                                                                         basis,
                                                                         valueDate,
                                                                         globalComponentCache)
                                                   );
                instrumentList.add(instrument);
            }
        }
    }

    void CashInstrument::updateInstruments(CalibrationInstruments& instrumentList, 
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
			//	Description/Tenor (e.g.: 3M)
            const string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(CASH, TENOR), i));

			if(!description.empty())
            {
                const double cashRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(CASH, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(cashRate);
                ++(*instrumentIndex);
            }
        }
    }

    double CashInstrument::getTenor() const
    {
        return m_forwardRate->getTenor();
    }
    
    double CashInstrument::getCoverage() const
    {
        return m_forwardRate->getCoverage();
    }
    
    const double CashInstrument::computeModelPrice(const BaseModelPtr model) const
    {
        return m_forwardRate->getValue(*model);
    }

    const double CashInstrument::computePV(const BaseModelPtr model) const
    {
        return m_forwardRate->getCoverage( ) * getResidual( model ) * model->getDiscountFactor( getLastRelevantTime() );
    }
	 
    const double CashInstrument::computeBPV(const BaseModelPtr model) const
    {
        return - oneBasisPoint( ) * m_forwardRate->getCoverage( ) * model->getDiscountFactor( getLastRelevantTime() );
    }

    void CashInstrument::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {   
        // compute the gradient of the forward
        m_forwardRate->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd);
    }

	void CashInstrument::accumulateGradient(BaseModel const& baseModel,
											double multiplier,
											GradientIterator gradientBegin,
											GradientIterator gradientEnd,
											const CurveTypeConstPtr& curveType)
	{
		m_forwardRate->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);
	}

	void CashInstrument::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void CashInstrument::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
    void CashInstrument::update()
    {
        m_forwardRate->update();
    }

    double CashInstrument::getLastRelevantTime() const 
    {
        return m_forwardRate->getLastRelevantTime(); 
    }
	
    double CashInstrument::getVariableInitialGuess(const double flowTime,
												   const BaseModel* const model) const
	{
		LT_LOG << "initial guess for C" << getDescription().string() << ", " << getDescription().string() << std::endl;
		// Should take into account to some account when the 3M cash knot-point
		//	is being placed at the start date of the first future
		return model->getVariableValueFromSpineDiscountFactor(flowTime, 1.0 / (1.0 + flowTime * getRate()), CurveType::getFromDescription(tenorEquivalency(getDescription().string())));
	}

	double CashInstrument::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return -1.0; 
	}
    
	void CashInstrument::accumulateRateDerivativeGradient(const BaseModel& model,
														  double multiplier,
														  GradientIterator gradientBegin,
														  GradientIterator gradientEnd) const
	{
		//	Do nothing
	}

	void CashInstrument::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		//	Do nothing
	}

	double CashInstrument::computeParRate(const BaseModelPtr& model)
	{
		return m_forwardRate->getValue(*model);
	}
	
	void CashInstrument::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                      const BaseModel& model,
									  const double multiplier, 
									  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		m_forwardRate->fillRepFlows(assetDomain, model, getCoverage() * multiplier * model.getDiscountFactor( getLastRelevantTime()), indexRepFlows);
	}
	
	void CashInstrument::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                      const BaseModel& model,
									  const double multiplier, 
									  IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
	}
	
	double CashInstrument::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		LTQuant::GenericDataPtr instrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(instrumentListData, getKey<CashInstrument>(), instrumentData);

		if(!instrumentData)
		{
			LTQC_THROW( LTQC::DataQCException, "Cannot find the instrument table \"" << getKey<CashInstrument>().getName().string() << "\"" );
		}

		double newRate;
		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentData));
		
        const DictionaryKey& cashTenor = IDeA_KEY(CASH, TENOR);
        const DictionaryKey& cashRate = IDeA_KEY(CASH, RATE);

		for(size_t i(0); i < nbInstruments; ++i)
		{
			if(getDescription().compareCaseless("O/N") == 0 && ( IDeA::extract<std::string>(*instrumentData,cashTenor, i).compare("O/N") == 0 ||   IDeA::extract<std::string>(*instrumentData, cashTenor, i).compare("1D") == 0 ) )
			{
				newRate = IDeA::extract<double>(*instrumentData, cashRate, i);
				return newRate - getRate();
			}
			if(getDescription().compareCaseless("T/N") == 0 && ( IDeA::extract<std::string>(*instrumentData,cashTenor, i).compare("T/N") == 0 ||   IDeA::extract<std::string>(*instrumentData, cashTenor, i).compare("2D") == 0 ) )
			{
				newRate = IDeA::extract<double>(*instrumentData, cashRate, i);
				return newRate - getRate();
			}
			if(compareAndTryGetNewRate(*instrumentData, i, cashTenor, cashRate, newRate))
			{
				return newRate - getRate();
			}
		}
		
		LTQC_THROW( LTQC::DataQCException, "Cannot find an instrument with description \"" << getDescription().string() << "\" in table \"" <<  getKey<CashInstrument>().getName().string() << "\"" );
	}


	ostream& CashInstrument::print(ostream& out) const
    {
        out << "C" << getDescription().string();
        return out;
    }

    /**
        @brief Clone this cash instrument.

        The original and clone will share InstrumentComponent's as these will not be altered in the clone.
    */
    ICloneLookupPtr CashInstrument::cloneWithLookup(CloneLookup& lookup) const
    {
        CashInstrumentPtr clone(new CashInstrument(*this));
        clone->m_forwardRate = lookup.get(m_forwardRate);
        return clone;
    }
}
