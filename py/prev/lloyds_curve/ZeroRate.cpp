#include "stdafx.h"

// FlexYCF
#include "ZeroRate.h"
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
	const IDeA::DictionaryKey& getKey<ZeroRate>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, ZERORATE);
	}

    using namespace LTQuant;
    using namespace ModuleDate;

    ZeroRate::ZeroRate(const string& description, const LT::date endDate, const double rate, const LT::date valueDate, GlobalComponentCache& globalComponentCache) :
     CalibrationInstrument(rate, getKeyName<ZeroRate>(), description, valueDate, valueDate, endDate), m_discountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate)))
    {
    }

	ZeroRate::ZeroRate(const std::string& tenorDescription, const LT::date endDate, const double rate, const LT::date valueDate) :
     CalibrationInstrument(rate, getKeyName<ZeroRate>(), tenorDescription, valueDate, valueDate, endDate), m_discountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate)))
    {
    }
    
	ZeroRate::~ZeroRate()
    {
    }

	CalibrationInstrumentPtr ZeroRate::create(const LTQuant::GenericData& instrumentParametersTable,
											  const LT::Ptr<LT::date>& buildDate,
											  const LTQuant::GenericData& curveParametersTable)
	{
		LT::date endDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE));
		return CalibrationInstrumentPtr(new ZeroRate(endDate.toDDMMMYYYYString(),endDate,0.0, *buildDate));
	}

    void ZeroRate::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_discountFactor.reset();
    }

    void ZeroRate::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {

        const ZeroRatePtr ourTypeSrc=std::tr1::static_pointer_cast<ZeroRate>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        m_discountFactor=ourTypeSrc->m_discountFactor;
        CalibrationInstrument::reloadInternalState(src);
    }

    void ZeroRate::createInstruments(CalibrationInstruments& instrumentList, 
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
		
		std::string calendar, emptyStr;
		IDeA::permissive_extract<std::string>(parametersTable, IDeA_KEY(YC_CURVEPARAMETERS, HOLIDAYCALENDAR), calendar, emptyStr);
        
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


                CalibrationInstrumentPtr instrument( new ZeroRate( foundTenor ? description : endDate.toDDMMMYYYYString(), endDate, rate, valueDate, globalComponentCache) );
                instrumentList.add(instrument);
            }
			else
				LTQC_THROW(IDeA::MarketException, "Neither tenor or end date is provided.");
        }
    }

    void ZeroRate::updateInstruments(CalibrationInstruments& instrumentList, 
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

    double ZeroRate::getTenor() const
    {
        return m_discountFactor->getArguments().getFlowTime();
    }
    
    
    const double ZeroRate::computeModelPrice(const BaseModelPtr model) const
    {
        double df = m_discountFactor->getValue(*model);
        double t  = getTenor();
        if( t > 0.0 )
            return - ::log(df)/t;
        else
            return 0.0;
    }

    const double ZeroRate::computePV(const BaseModelPtr model) const
    {
        return getResidual( model );
    }

    const double ZeroRate::computeBPV(const BaseModelPtr model) const
    {
        return (::exp(- oneBasisPoint( ) * getTenor() ) -1.0) * m_discountFactor->getValue(*model);
    }

    void ZeroRate::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {   
        double df = m_discountFactor->getValue(baseModel);
        double t  = getTenor();
        if( t > 0.0 )
            m_discountFactor->accumulateGradient(baseModel, - multiplier/(df*t), gradientBegin, gradientEnd);
    }

	void ZeroRate::accumulateGradient(BaseModel const& baseModel,
											double multiplier,
											GradientIterator gradientBegin,
											GradientIterator gradientEnd,
											const CurveTypeConstPtr& curveType)
	{
        double df = m_discountFactor->getValue(baseModel);
        double t  = getTenor();
        if( t > 0.0 )
            m_discountFactor->accumulateGradient(baseModel, -multiplier/(df*t), gradientBegin, gradientEnd, curveType);
	}
	
	void ZeroRate::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void ZeroRate::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void ZeroRate::update()
    {
        m_discountFactor->update();
    }

    double ZeroRate::getLastRelevantTime() const 
    {
        return getTenor(); 
    }
	
    double ZeroRate::getVariableInitialGuess(const double flowTime, const BaseModel* const model) const
	{
		LT_LOG << "initial guess for Z" << getDescription().string() << ", " << getDescription().string() << std::endl;
		return model->getVariableValueFromSpineDiscountFactor(flowTime, ::exp(- flowTime * getRate() ), CurveType::_3M());
	}

	double ZeroRate::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return -getTenor() * m_discountFactor->getValue(*model);
	}
    
	void ZeroRate::accumulateRateDerivativeGradient(const BaseModel& model,
														  double multiplier,
														  GradientIterator gradientBegin,
														  GradientIterator gradientEnd) const
	{
        double d = -getTenor() * m_discountFactor->getValue(model);
        m_discountFactor->accumulateGradient(model, multiplier * d, gradientBegin, gradientEnd);
	}

	void ZeroRate::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
        double d = -getTenor() * m_discountFactor->getValue(model);
        m_discountFactor->accumulateGradient(model, multiplier * d, gradientBegin, gradientEnd, curveType);
	}

	double ZeroRate::computeParRate(const BaseModelPtr& model)
	{ 
        double df = m_discountFactor->getValue(*model);
        double t  = getTenor();
        if( t > 0.0 )
            return - ::log(df)/t;
        else
            return 0.0;
	}
	
	void ZeroRate::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                      const BaseModel& model,
									  const double multiplier, 
									  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		LTQC_THROW(IDeA::MarketException, "FillRepFlows not implemented for zero rate");
	}
	
	double ZeroRate::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		LTQuant::GenericDataPtr instrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(instrumentListData, getKey<ZeroRate>(), instrumentData);

		if(!instrumentData)
		{
			LTQC_THROW( LTQC::DataQCException, "Cannot find the instrument table \"" << getKeyName<ZeroRate>().string() << "\"" );
		}

		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentData));
		for(size_t i = 0; i < nbInstruments; ++i)
		{
            std::string description, emptyStr;
            LT::date endDate;

            bool foundTenor = IDeA::permissive_extract<std::string>(*instrumentData, IDeA_KEY(ZERORATE, TENOR), i, description, emptyStr);	
            bool foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentData, IDeA_KEY(ZERORATE, ENDDATE), i, endDate, LT::date() );
			if(getDescription().compareCaseless("O/N") == 0 && ( IDeA::extract<std::string>(*instrumentData,IDeA_KEY(ZERORATE, TENOR), i).compare("O/N") == 0 ||   IDeA::extract<std::string>(*instrumentData, IDeA_KEY(ZERORATE, TENOR), i).compare("1D") == 0 ) )
			{
				const double newRate = IDeA::extract<double>(*instrumentData, IDeA_KEY(ZERORATE, RATE), i);
				return newRate - getRate();
			}

			if(getDescription().compareCaseless("T/N") == 0 && ( IDeA::extract<std::string>(*instrumentData,IDeA_KEY(ZERORATE, TENOR), i).compare("T/N") == 0 ||   IDeA::extract<std::string>(*instrumentData, IDeA_KEY(ZERORATE, TENOR), i).compare("2D") == 0 ) )
			{
				const double newRate = IDeA::extract<double>(*instrumentData, IDeA_KEY(ZERORATE, RATE), i);
				return newRate - getRate();
			}

			if( (foundTenor && (getDescription().compareCaseless(description) == 0)) || ( foundEndDate &&  this->getDescription().compareCaseless(endDate.toDDMMMYYYYString()) == 0 ) )
			{
				const double newRate = IDeA::extract<double>(*instrumentData, IDeA_KEY(ZERORATE, RATE),i);
				return newRate - getRate();					
			}
		}
           
        LTQC_THROW( LTQC::DataQCException, "Cannot find an instrument with description \"" << getDescription().string() << "\" in table \"" << getKeyName<ZeroRate>().string() << "\"" );
	}
	
	ostream& ZeroRate::print(ostream& out) const
    {
        out << "Z" << getDescription().string();
        return out;
    }

    ICloneLookupPtr ZeroRate::cloneWithLookup(CloneLookup& lookup) const
    {
        ZeroRatePtr clone(new ZeroRate(*this));
        clone->m_discountFactor = lookup.get(m_discountFactor);
        return clone;
    }
}
