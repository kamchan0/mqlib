#include "stdafx.h"

// FlexYCF
#include "TenorZeroRate.h"
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
	template<>
	const IDeA::DictionaryKey& getKey<TenorZeroRate>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, TENORZERORATES);
	}

    using namespace LTQuant;
    using namespace ModuleDate;

    TenorZeroRate::TenorZeroRate(const string& description, const std::string& tenorDescription, const LT::date endDate, const double rate, const LT::date valueDate, GlobalComponentCache& globalComponentCache) :
     CalibrationInstrument(rate, getKeyName<TenorZeroRate>(), description, valueDate, valueDate, endDate), m_tenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(valueDate, endDate,tenorDescToYears(tenorDescription))))
    {
    }

	TenorZeroRate::TenorZeroRate(const string& description, const std::string& tenorDescription, const LT::date endDate, const double rate, const LT::date valueDate) :
     CalibrationInstrument(rate, getKeyName<TenorZeroRate>(), description, valueDate, valueDate, endDate), m_tenorDiscountFactor(TenorDiscountFactor::create(TenorDiscountFactorArguments(valueDate, endDate, tenorDescToYears(tenorDescription))))
    {
    }
    
	TenorZeroRate::~TenorZeroRate()
    {
    }

	CalibrationInstrumentPtr TenorZeroRate::create(const LTQuant::GenericData& instrumentParametersTable,
											  const LT::Ptr<LT::date>& buildDate,
											  const LTQuant::GenericData& curveParametersTable)
	{
		LT::date endDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE));
		std::string tenor = IDeA::extract<std::string>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, TENOR1));
		return CalibrationInstrumentPtr(new TenorZeroRate(endDate.toDDMMMYYYYString(),tenor,endDate,0.0, *buildDate));
	}

    void TenorZeroRate::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_tenorDiscountFactor.reset();
    }

    void TenorZeroRate::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {

        const TenorZeroRatePtr ourTypeSrc=std::tr1::static_pointer_cast<TenorZeroRate>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        m_tenorDiscountFactor=ourTypeSrc->m_tenorDiscountFactor;
        CalibrationInstrument::reloadInternalState(src);
    }

    void TenorZeroRate::createInstruments(CalibrationInstruments& instrumentList, 
                                           LTQuant::GenericDataPtr instrumentsTable,
                                           LTQuant::GenericDataPtr masterTable,
                                           GlobalComponentCache& globalComponentCache,
                                           const LTQuant::PriceSupplierPtr)
    {
        // just finish if we have empty table or just headings
        if(instrumentsTable->numItems() < 2)
        {
            return;
        }

        const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
		const GenericDataPtr parametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
		
		std::string calendar, emptyStr;
		IDeA::permissive_extract<std::string>(parametersTable, IDeA_KEY(YC_CURVEPARAMETERS, HOLIDAYCALENDAR), calendar, emptyStr);
       
		LT::TablePtr table = instrumentsTable->table;
		size_t rows = table->rowsGet();
		size_t k1 = table->findColKey(IDeA_KEY(TENORZERORATES, TENOR).getName());
		size_t k2 = table->findColKey(IDeA_KEY(TENORZERORATES, ZERORATE).getName());

		for(size_t j = 1; j < rows; ++j)
		{	
			LT::TablePtr zeroRatesTable = table->at(j,k2);
			LT::Str tenorStr = table->at(j,k1);
			std::string tenorDescription(tenorStr.cStr());
			LTQuant::GenericDataPtr instrumentTable(new GenericData(zeroRatesTable));

			for(size_t i(0); i < IDeA::numberOfRecords(*instrumentTable); ++i)
			{
				LT::date endDate;
				std::string description;
				bool foundTenor = IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, TENOR), i, description, emptyStr);	
				bool foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentTable, IDeA_KEY(ZERORATE, ENDDATE), i, endDate, LT::date() );
			
				if(foundTenor && foundEndDate)
					LTQC_THROW(IDeA::MarketException, "Tenor or end Date have to be provided but not both");

				if( foundTenor || foundEndDate)
				{
					const double rate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(ZERORATE, RATE), i));
					if(foundTenor) 
					{
						std::string rollConv, rollRuleConv, calendarEnd, spotRollConv, spotRollRuleConv, spotCalendar, spotDays;
					
						IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, ROLLCONVENTION), i, rollConv, emptyStr);
						rollConv = (rollConv != emptyStr) ? rollConv : LTQC::RollConvMethod(RollConvMethod::None).asString().data();

						IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, ROLLRULECONVENTION), i, rollRuleConv, emptyStr);
						rollRuleConv = (rollRuleConv != emptyStr) ? rollRuleConv : LTQC::RollRuleMethod(RollRuleMethod::None).asString().data();

						IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, CALENDAR), i, calendarEnd, emptyStr);
						calendarEnd = (calendarEnd != emptyStr) ? calendarEnd : calendar;

 						IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, SPOTDAYS), i, spotDays, emptyStr);
						spotDays = (spotDays != emptyStr) ? spotDays : "0C";

						IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, SPOTROLLCONVENTION), i, spotRollConv, emptyStr);
						spotRollConv = (spotRollConv != emptyStr) ? spotRollConv : LTQC::RollConvMethod(RollConvMethod::None).asString().data();

						IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, SPOTROLLRULECONVENTION), i, spotRollRuleConv, emptyStr);
						spotRollRuleConv = (spotRollRuleConv != emptyStr) ? spotRollRuleConv : LTQC::RollRuleMethod(RollRuleMethod::None).asString().data();

						IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, SPOTCALENDAR), i, spotCalendar, emptyStr);
						spotCalendar = (spotCalendar != emptyStr) ? spotCalendar : calendarEnd;

						const Date spotDate = LTQC::DateBuilder::dateAdd(Date(valueDate), spotDays, spotCalendar, spotRollConv, spotRollRuleConv, LTQC::RollConvMethod(RollConvMethod::None).asString().data());
						endDate = LTQC::DateBuilder::dateAdd(spotDate, description, calendarEnd, rollConv, rollRuleConv, LTQC::RollConvMethod(RollConvMethod::None).asString().data()).getAsLTdate();
					}                   
					CalibrationInstrumentPtr instrument( new TenorZeroRate( foundTenor ? description : endDate.toDDMMMYYYYString(), tenorDescription, endDate, rate, valueDate, globalComponentCache) );
					instrumentList.add(instrument);
				}
				else
					LTQC_THROW(IDeA::MarketException, "Neither tenor or end date is provided.");
			}
		}
    }

    void TenorZeroRate::updateInstruments(CalibrationInstruments& instrumentList, 
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
            bool foundTenor = IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZERORATE, TENOR), i, description, emptyStr);	
            bool foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentTable, IDeA_KEY(ZERORATE, ENDDATE), i, endDate, LT::date() );
			
            if(foundTenor || foundEndDate)
            {
                const double zeroRate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(ZERORATE, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(zeroRate);
                ++(*instrumentIndex);
            }
        }
    }

    double TenorZeroRate::getTenor() const
    {
        return m_tenorDiscountFactor->getArguments().getFlowTime();
    }
    
    CurveTypeConstPtr TenorZeroRate::getCurveType() const
	{ 
		return CurveType::getFromYearFraction(m_tenorDiscountFactor->getArguments().getTenor());
	}

    const double TenorZeroRate::computeModelPrice(const BaseModelPtr model) const
    {
        double df = m_tenorDiscountFactor->getValue(*model);
        double t  = getTenor();
        if( t > 0.0 )
            return - ::log(df)/t;
        else
            return 0.0;
    }

    const double TenorZeroRate::computePV(const BaseModelPtr model) const
    {
        return getResidual( model );
    }

    const double TenorZeroRate::computeBPV(const BaseModelPtr model) const
    {
        return (::exp(- oneBasisPoint( ) * getTenor() ) -1.0) * m_tenorDiscountFactor->getValue(*model);
    }

    void TenorZeroRate::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {   
        double df = m_tenorDiscountFactor->getValue(baseModel);
        double t  = getTenor();
        if( t > 0.0 )
            m_tenorDiscountFactor->accumulateGradient(baseModel, - multiplier/(df*t), gradientBegin, gradientEnd);
    }

	void TenorZeroRate::accumulateGradient(BaseModel const& baseModel,
											double multiplier,
											GradientIterator gradientBegin,
											GradientIterator gradientEnd,
											const CurveTypeConstPtr& curveType)
	{
        double df = m_tenorDiscountFactor->getValue(baseModel);
        double t  = getTenor();
        if( t > 0.0 )
            m_tenorDiscountFactor->accumulateGradient(baseModel, -multiplier/(df*t), gradientBegin, gradientEnd, curveType);
	}
	
	void TenorZeroRate::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void TenorZeroRate::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void TenorZeroRate::update()
    {
        m_tenorDiscountFactor->update();
    }

    double TenorZeroRate::getLastRelevantTime() const 
    {
        return getTenor(); 
    }
	
    double TenorZeroRate::getVariableInitialGuess(const double flowTime, const BaseModel* const model) const
	{
		LT_LOG << "initial guess for TZ" << getDescription().string() << ", " << getDescription().string() << std::endl;
		return model->getVariableValueFromSpineDiscountFactor(flowTime, ::exp(- flowTime * getRate() ), CurveType::getFromYearFraction(m_tenorDiscountFactor->getArguments().getTenor()));
	}

	double TenorZeroRate::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return -getTenor() * m_tenorDiscountFactor->getValue(*model);
	}
    
	void TenorZeroRate::accumulateRateDerivativeGradient(const BaseModel& model,
														  double multiplier,
														  GradientIterator gradientBegin,
														  GradientIterator gradientEnd) const
	{
        double d = -getTenor() * m_tenorDiscountFactor->getValue(model);
        m_tenorDiscountFactor->accumulateGradient(model, multiplier * d, gradientBegin, gradientEnd);
	}

	void TenorZeroRate::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
        double d = -getTenor() * m_tenorDiscountFactor->getValue(model);
        m_tenorDiscountFactor->accumulateGradient(model, multiplier * d, gradientBegin, gradientEnd, curveType);
	}

	double TenorZeroRate::computeParRate(const BaseModelPtr& model)
	{ 
        double df = m_tenorDiscountFactor->getValue(*model);
        double t  = getTenor();
        if( t > 0.0 )
            return - ::log(df)/t;
        else
            return 0.0;
	}
	
	void TenorZeroRate::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                      const BaseModel& model,
									  const double multiplier, 
									  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		LTQC_THROW(IDeA::MarketException, "FillRepFlows not implemented for zero rate");
	}
	
	double TenorZeroRate::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		LTQuant::GenericDataPtr instrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(instrumentListData, getKey<TenorZeroRate>(), instrumentData);

		if(!instrumentData)
		{
			LTQC_THROW( LTQC::DataQCException, "Cannot find the instrument table \"" << getKeyName<TenorZeroRate>().string() << "\"" );
		}

		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentData));
		for(size_t i = 0; i < nbInstruments; ++i)
		{
            std::string description, emptyStr;
            LT::date endDate;

            bool foundTenor = IDeA::permissive_extract<std::string>(*instrumentData, IDeA_KEY(ZERORATE, TENOR), i, description, emptyStr);	
            bool foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentData, IDeA_KEY(ZERORATE, ENDDATE), i, endDate, LT::date() );
			if( (foundTenor && description == this->getDescription().string()) || ( foundEndDate &&  endDate.toDDMMMYYYYString() == this->getDescription().string() ) )
			{
				const double newRate(IDeA::extract<double>(*instrumentData, IDeA_KEY(ZERORATE, RATE)));
				return newRate - this->getRate();					
			}
            else 
            {
                LTQC_THROW( LTQC::DataQCException, "Cannot find an instrument with description \"" << getDescription().string() << "\" in table \"" << getKeyName<TenorZeroRate>().string() << "\"" );
            }
		}
        return 0.0;
	}
	
	ostream& TenorZeroRate::print(ostream& out) const
    {
        out << "TZ" << getDescription().string();
        return out;
    }

    ICloneLookupPtr TenorZeroRate::cloneWithLookup(CloneLookup& lookup) const
    {
        TenorZeroRatePtr clone(new TenorZeroRate(*this));
        clone->m_tenorDiscountFactor = lookup.get(m_tenorDiscountFactor);
        return clone;
    }
}
