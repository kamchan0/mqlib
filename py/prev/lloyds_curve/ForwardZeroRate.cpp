/**
	@file Implement the ForwardZeroRate

    @author Chongxian Zhu

    @ownergroup IDeA

    Copyright (C) Lloyds TSB Group plc 2013 All Rights Reserved
*/

#include "stdafx.h"

// FlexYCF
#include "ForwardZeroRate.h"
#include "CalibrationInstruments.h"
#include "BaseModel.h"
#include "GlobalComponentCache.h"
#include "TenorUtils.h"
#include "DataExtraction.h"
#include "FlexYcfUtils.h"
#include "FlexYCFCloneLookup.h"

// IDeA
#include "AssetDomain.h"
#include "DictYieldCurve.h"

// LTQuantCore
#include "dates\DateBuilder.h"

// LTQuantLib
#include "Data/GenericData.h"
#include "DateUtils.h"


using namespace std;

using namespace LTQC;

namespace FlexYCF
{
	namespace 
	{
		void retrieveStartEndFromInstrumentList(LT::date& startDate, 
												LT::date& endDate, 
												LT::Str& startTenorStr, 
												LT::Str& startToEndTenorStr, 
												bool& foundTenor, 
												bool& foundEndDate, 
												bool& startDateAsTenor, 
												LT::Str& description,
												const LTQuant::GenericDataPtr& instrumentTable,
												const size_t i)
		{
			LT::Cell startDateAsCell;
			std::string emptyStr;

			// handle end date
			foundTenor = IDeA::permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(FORWARDZERORATE, TENOR), i, startToEndTenorStr, emptyStr);	
			foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentTable, IDeA_KEY(FORWARDZERORATE, ENDDATE), i, endDate, LT::date() );
			if(foundTenor && foundEndDate)
				LTQC_THROW(IDeA::MarketException, "Tenor or end Date have to be provided but not both");

			// handle start date
			IDeA::permissive_extract< LT::Cell >( instrumentTable->table, IDeA_KEY(FORWARDZERORATE, STARTDATE ), i, startDateAsCell, LT::Cell( ) );	
			static const LT::Str errorMsg("Failed to convert date");
			if( startDateAsCell.valueType( ) == LT::Cell::string_type)
			{
				startTenorStr = extract( startDateAsCell ) || errorMsg.data( );
				startDateAsTenor = true;
			}
			else
			{
				startDate = extractDate( startDateAsCell, errorMsg ).getAsLTdate();
				startDateAsTenor = false;
			}

			// make the unique description
			const LT::Str startDescription = startDateAsTenor ? startTenorStr : startDate.toDDMMMYYYYString();
			const LT::Str endDescription = foundTenor ? startToEndTenorStr : endDate.toDDMMMYYYYString();
			description = startDescription + "x" + endDescription;
		}
	}

	template<>
	const IDeA::DictionaryKey& getKey<ForwardZeroRate>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, FORWARDZERORATE);
	}

    using namespace LTQuant;
    using namespace ModuleDate;

    ForwardZeroRate::ForwardZeroRate(const std::string& description, const LT::date startDate, const LT::date endDate, const double rate, const LT::date valueDate, GlobalComponentCache& globalComponentCache) 
		:	CalibrationInstrument(rate, getKeyName<ForwardZeroRate>(), description, valueDate, startDate, endDate),
			m_discountFactorOnStartDate(DiscountFactor::create(DiscountFactorArguments(valueDate, startDate))),
			m_discountFactorOnEndDate(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate)))
    {
    }

	ForwardZeroRate::ForwardZeroRate(const std::string& description, const LT::date startDate, const LT::date endDate, const double rate, const LT::date valueDate)		
		:	CalibrationInstrument(rate, getKeyName<ForwardZeroRate>(), description, valueDate, startDate, endDate),
			m_discountFactorOnStartDate(DiscountFactor::create(DiscountFactorArguments(valueDate, startDate))),
			m_discountFactorOnEndDate(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate)))
    {
    }
    
	ForwardZeroRate::~ForwardZeroRate()
    {
    }

	CalibrationInstrumentPtr ForwardZeroRate::create(	const LTQuant::GenericData& instrumentParametersTable,
														const LT::Ptr<LT::date>& buildDate,
														const LTQuant::GenericData& curveParametersTable)
	{
		const LT::date endDate	= IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE));
		const LT::date startDate	= IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, STARTDATE));

		const LT::Str description = startDate.toDDMMMYYYYString() + "x" + endDate.toDDMMMYYYYString();
		return CalibrationInstrumentPtr(new ForwardZeroRate(description.string(), startDate, endDate, 0.0, *buildDate));
	}

    void ForwardZeroRate::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
		m_discountFactorOnStartDate.reset();
		m_discountFactorOnEndDate.reset();
    }

	void ForwardZeroRate::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {

        const ForwardZeroRatePtr ourTypeSrc=std::tr1::static_pointer_cast<ForwardZeroRate>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }
		
		m_discountFactorOnStartDate = ourTypeSrc->m_discountFactorOnStartDate;
		m_discountFactorOnEndDate = ourTypeSrc->m_discountFactorOnEndDate;
        CalibrationInstrument::reloadInternalState(src);
    }

    void ForwardZeroRate::createInstruments(CalibrationInstruments& instrumentList, 
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
		const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		
		LT::Str calendar, emptyStr;
		IDeA::permissive_extract<LT::Str>(parametersTable->table, IDeA_KEY(YC_CURVEPARAMETERS, HOLIDAYCALENDAR), calendar, emptyStr);
        
        for(size_t i(0); i < IDeA::numberOfRecords(*instrumentTable); ++i)
        {
			LT::date startDate, endDate;
			LT::Str startTenorStr, startToEndTenorStr, description;
			bool startDateAsTenor, foundTenor, foundEndDate;
			
			retrieveStartEndFromInstrumentList(startDate, endDate, startTenorStr, startToEndTenorStr, foundTenor, foundEndDate, startDateAsTenor, description, instrumentTable, i);

			// compute final start / end dates			
			if( foundTenor || foundEndDate)
            {
                const double rate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(FORWARDZERORATE, RATE), i));
				
				if(startDateAsTenor || foundTenor)
				{
					LT::Str rollConv, rollRuleConv, calendarEnd, spotRollConv, spotRollRuleConv, spotCalendar, spotDays;

					IDeA::permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(FORWARDZERORATE, ROLLCONVENTION), i, rollConv, emptyStr);
					rollConv = (rollConv != emptyStr) ? rollConv : LTQC::RollConvMethod(RollConvMethod::None).asString().data();

					IDeA::permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(FORWARDZERORATE, ROLLRULECONVENTION), i, rollRuleConv, emptyStr);
					rollRuleConv = (rollRuleConv != emptyStr) ? rollRuleConv : LTQC::RollRuleMethod(RollRuleMethod::None).asString().data();

					IDeA::permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(FORWARDZERORATE, CALENDAR), i, calendarEnd, emptyStr);
					calendarEnd = (calendarEnd != emptyStr) ? calendarEnd : calendar;

					IDeA::permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(FORWARDZERORATE, SPOTDAYS), i, spotDays, emptyStr);
					spotDays = (spotDays != emptyStr) ? spotDays : "0C";

					IDeA::permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(FORWARDZERORATE, SPOTROLLCONVENTION), i, spotRollConv, emptyStr);
					spotRollConv = (spotRollConv != emptyStr) ? spotRollConv : LTQC::RollConvMethod(RollConvMethod::None).asString().data();

					IDeA::permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(FORWARDZERORATE, SPOTROLLRULECONVENTION), i, spotRollRuleConv, emptyStr);
					spotRollRuleConv = (spotRollRuleConv != emptyStr) ? spotRollRuleConv : LTQC::RollRuleMethod(RollRuleMethod::None).asString().data();

					IDeA::permissive_extract<LT::Str>(instrumentTable->table, IDeA_KEY(FORWARDZERORATE, SPOTCALENDAR), i, spotCalendar, emptyStr);
					spotCalendar = (spotCalendar != emptyStr) ? spotCalendar : calendarEnd;

					const Date spotDate = LTQC::DateBuilder::dateAdd(Date(valueDate), spotDays, spotCalendar, spotRollConv, spotRollRuleConv, LTQC::RollConvMethod(RollConvMethod::None).asString().data());

					if (startDateAsTenor)
						startDate = LTQC::DateBuilder::dateAdd(spotDate, startTenorStr, calendarEnd, rollConv, rollRuleConv, LTQC::RollConvMethod(RollConvMethod::None).asString().data()).getAsLTdate();

					if (foundTenor)
						endDate = LTQC::DateBuilder::dateAdd(startDate, startToEndTenorStr, calendarEnd, rollConv, rollRuleConv, LTQC::RollConvMethod(RollConvMethod::None).asString().data()).getAsLTdate();
				}                   

                CalibrationInstrumentPtr instrument( new ForwardZeroRate( description.string(), startDate, endDate, rate, valueDate, globalComponentCache) );
                instrumentList.add(instrument);
            }
			else
				LTQC_THROW(IDeA::MarketException, "Neither tenor or end date is provided.");
        }
    }

    void ForwardZeroRate::updateInstruments(CalibrationInstruments& instrumentList, 
											LTQuant::GenericDataPtr instrumentTable, 
											size_t* instrumentIndex)
    {
        // just finish if we have empty table or just headings
        if(instrumentTable->numItems() < 2)
        {
            return;
        }

		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentTable));
        for(size_t i = 0; i < nbInstruments; ++i)
        {
            LT::date endDate;
			std::string description, emptyStr;
            bool foundTenor = IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(FORWARDZERORATE, TENOR), i, description, emptyStr);	
            bool foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentTable, IDeA_KEY(FORWARDZERORATE, ENDDATE), i, endDate, LT::date() );
			
            if(foundTenor || foundEndDate)
            {
                const double zeroRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(FORWARDZERORATE, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(zeroRate);
                ++(*instrumentIndex);
            }
        }
    }

	const double ForwardZeroRate::computeModelPrice(const BaseModelPtr model) const
	{
		const double df1 = m_discountFactorOnStartDate->getValue(*model);
		const double df2 = m_discountFactorOnEndDate->getValue(*model);
		const double t1 = m_discountFactorOnStartDate->getArguments().getFlowTime();
		const double t2 = m_discountFactorOnEndDate->getArguments().getFlowTime();

		if( t2 > 0.0 )
			if (t1 > 0.0)	return - ::log(df2/df1)/(t2-t1);
			else			return - ::log(df2)/t2;
		else
			return 0.0;
	}

    void ForwardZeroRate::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {   
		const double df1 = m_discountFactorOnStartDate->getValue(baseModel);
		const double df2 = m_discountFactorOnEndDate->getValue(baseModel);
		const double t1 = m_discountFactorOnStartDate->getArguments().getFlowTime();
		const double t2 = m_discountFactorOnEndDate->getArguments().getFlowTime();

		if( t2 > 0.0 )
			if (t1 > 0.0)	
			{
				const double cvg = t2 - t1;
				m_discountFactorOnStartDate->accumulateGradient(baseModel, multiplier / (cvg * df1), gradientBegin, gradientEnd );
				m_discountFactorOnEndDate->accumulateGradient( baseModel, -multiplier / (cvg * df2), gradientBegin, gradientEnd );
			}
			else
			{
				m_discountFactorOnEndDate->accumulateGradient(baseModel, - multiplier/(df2 * t2), gradientBegin, gradientEnd);
			}
	}

	void ForwardZeroRate::accumulateGradient(	BaseModel const& baseModel,
												double multiplier,
												GradientIterator gradientBegin,
												GradientIterator gradientEnd,
												const CurveTypeConstPtr& curveType)
	{
		const double df1 = m_discountFactorOnStartDate->getValue(baseModel);
		const double df2 = m_discountFactorOnEndDate->getValue(baseModel);
		const double t1 = m_discountFactorOnStartDate->getArguments().getFlowTime();
		const double t2 = m_discountFactorOnEndDate->getArguments().getFlowTime();

		if( t2 > 0.0 )
			if (t1 > 0.0)	
			{
				const double cvg = t2 - t1;
				m_discountFactorOnStartDate->accumulateGradient(baseModel, multiplier / (cvg * df1), gradientBegin, gradientEnd, curveType );
				m_discountFactorOnEndDate->accumulateGradient( baseModel, -multiplier / (cvg * df2), gradientBegin, gradientEnd, curveType );
			}
			else
			{
				m_discountFactorOnEndDate->accumulateGradient(baseModel, - multiplier/(df2 * t2), gradientBegin, gradientEnd, curveType);
			}
	}
	
	void ForwardZeroRate::accumulateGradientConstantDiscountFactor(	BaseModel const& baseModel, 
																	BaseModel const& dfModel, 
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
    
	void ForwardZeroRate::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, 
																		BaseModel const& dfModel, 
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
    
    const double ForwardZeroRate::computePV(const BaseModelPtr model) const
    {
		const double t1 = m_discountFactorOnStartDate->getArguments().getFlowTime();
		const double t2 = m_discountFactorOnEndDate->getArguments().getFlowTime();
		const double modelRate = computeModelPrice(model);
		const double mktRate = getRate();
		const double cvg = (t2 > 0.0) 
							? ((t1 > 0.0) ? (t2 - t1) : t2) 
							: 0.0 ;
		return (::exp(modelRate * cvg) - ::exp(mktRate * cvg)) * ::exp(-modelRate * t2);
	}

    const double ForwardZeroRate::computeBPV(const BaseModelPtr model) const
    {
		const double t1 = m_discountFactorOnStartDate->getArguments().getFlowTime();
		const double t2 = m_discountFactorOnEndDate->getArguments().getFlowTime();
		const double modelRate = computeModelPrice(model);
		const double mktRate = getRate();
		const double cvg = (t2 > 0.0) 
							? ((t1 > 0.0) ? (t2 - t1) : t2) 
							: 0.0 ;
		return - oneBasisPoint() * cvg * ::exp(mktRate * cvg) * ::exp(-modelRate * t2);
    }

	void ForwardZeroRate::update()
    {
        m_discountFactorOnStartDate->update();
		m_discountFactorOnEndDate->update();
    }

    double ForwardZeroRate::getLastRelevantTime() const 
    {
        return m_discountFactorOnEndDate->getArguments().getFlowTime();
    }
	
    double ForwardZeroRate::getVariableInitialGuess(const double flowTime, const BaseModel* const model) const
	{
		LT_LOG << "initial guess for FwdZ" << getDescription().string() << ", " << getDescription().string() << std::endl;

		const CurveTypeConstPtr curveType = CurveType::_3M();
		const double t1 = m_discountFactorOnStartDate->getArguments().getFlowTime();
		const double t2 = m_discountFactorOnEndDate->getArguments().getFlowTime();
		
		if( t2 > 0.0 )
			if (t1 > 0.0)
			{
				const double cvg = t2 - t1;
				const double df1 = model->getSpineDiscountFactor(t1, curveType);
				return model->getVariableValueFromSpineDiscountFactor(t2, ::exp(-getRate() * cvg) * df1, curveType);
			}
			else
			{
				return model->getVariableValueFromSpineDiscountFactor(t2, ::exp(-getRate() * t2), curveType);
			}
		else
			return model->getVariableValueFromSpineDiscountFactor(0.0, 1.0, curveType);
	}

	double ForwardZeroRate::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return -1.0;
	}
    
	void ForwardZeroRate::accumulateRateDerivativeGradient(	const BaseModel& model,
															double multiplier,
															GradientIterator gradientBegin,
															GradientIterator gradientEnd) const
	{
		// do nothing
	}

	void ForwardZeroRate::accumulateRateDerivativeGradient(	const BaseModel& model,
															double multiplier,
															GradientIterator gradientBegin,
															GradientIterator gradientEnd,
															const CurveTypeConstPtr& curveType) const
	{
		// do nothing
	}

	double ForwardZeroRate::computeParRate(const BaseModelPtr& model)
	{ 
		return computeModelPrice(model);
	}
	
	void ForwardZeroRate::fillRepFlows(	IDeA::AssetDomainConstPtr assetDomain,
										const BaseModel& model,
										const double multiplier, 
										IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		LTQC_THROW(IDeA::MarketException, "FillRepFlows not implemented for forward zero rate");
	}

	void ForwardZeroRate::fillRepFlows( IDeA::AssetDomainConstPtr assetDomain, 
										const BaseModel& model,		
										const double multiplier, 
										IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows )
	{
		LTQC_THROW(IDeA::MarketException, "FillRepFlows not implemented for forward zero rate");
	}

	double ForwardZeroRate::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		LTQuant::GenericDataPtr instrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(instrumentListData, getKey<ForwardZeroRate>(), instrumentData);

		if(!instrumentData)
		{
			LTQC_THROW( LTQC::DataQCException, "Cannot find the instrument table \"" << getKeyName<ForwardZeroRate>().string() << "\"" );
		}

		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentData));
		for(size_t i = 0; i < nbInstruments; ++i)
		{
			LT::date startDate, endDate;
			LT::Str startTenorStr, startToEndTenorStr, description;
			bool startDateAsTenor, foundTenor, foundEndDate;
			
			retrieveStartEndFromInstrumentList(startDate, endDate, startTenorStr, startToEndTenorStr, foundTenor, foundEndDate, startDateAsTenor, description, instrumentData, i);

			if( getDescription().compareCaseless(description) == 0)
			{
				const double newRate = IDeA::extract<double>(*instrumentData, IDeA_KEY(FORWARDZERORATE, RATE),i);
				return newRate - getRate();					
			}
		}
           
        LTQC_THROW( LTQC::DataQCException, "Cannot find an instrument with description \"" << getDescription().string() << "\" in table \"" << getKeyName<ForwardZeroRate>().string() << "\"" );
	}
	
	ostream& ForwardZeroRate::print(ostream& out) const
    {
        out << "FwdZ" << getDescription().string();
        return out;
    }

    ICloneLookupPtr ForwardZeroRate::cloneWithLookup(CloneLookup& lookup) const
    {
        ForwardZeroRatePtr clone(new ForwardZeroRate(*this));
        clone->m_discountFactorOnStartDate = lookup.get(m_discountFactorOnStartDate);
		clone->m_discountFactorOnEndDate = lookup.get(m_discountFactorOnEndDate);
        return clone;
    }
}
