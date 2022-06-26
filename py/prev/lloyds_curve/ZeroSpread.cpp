#include "stdafx.h"

// FlexYCF
#include "ZeroSpread.h"
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
using namespace IDeA;

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<ZeroSpread>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, ZEROSPREAD);
	}

    using namespace LTQuant;
    using namespace ModuleDate;

    ZeroSpread::ZeroSpread(const string& description, const LT::date endDate, const double rate, const LT::date valueDate, GlobalComponentCache& globalComponentCache) :
     CalibrationInstrument(rate, getKeyName<ZeroSpread>(), description, valueDate, valueDate, endDate), 
		 m_discountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate))),
		 m_baseDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate," "," ")))
    {
    }

	ZeroSpread::ZeroSpread(const std::string& tenorDescription, const LT::date endDate, const double rate, const LT::date valueDate) :
     CalibrationInstrument(rate, getKeyName<ZeroSpread>(), tenorDescription, valueDate, valueDate, endDate), 
		 m_discountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate))),
		 m_baseDiscountFactor(DiscountFactor::create(DiscountFactorArguments(valueDate, endDate," "," ")))
    {
    }
    
	ZeroSpread::~ZeroSpread()
    {
    }

	CalibrationInstrumentPtr ZeroSpread::create(const LTQuant::GenericData& instrumentParametersTable,
											  const LT::Ptr<LT::date>& buildDate,
											  const LTQuant::GenericData& curveParametersTable)
	{
		LT::date endDate = IDeA::extract<LT::date>(instrumentParametersTable, IDeA_KEY(YCI_INSTRUMENTPARAMETERS, ENDDATE));
		return CalibrationInstrumentPtr(new ZeroSpread(endDate.toDDMMMYYYYString(),endDate,0.0, *buildDate));
	}

    void ZeroSpread::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_discountFactor.reset();
		m_baseDiscountFactor.reset();
    }

    void ZeroSpread::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {

        const ZeroSpreadPtr ourTypeSrc=std::tr1::static_pointer_cast<ZeroSpread>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        m_discountFactor=ourTypeSrc->m_discountFactor;
		m_baseDiscountFactor=ourTypeSrc->m_baseDiscountFactor;
        CalibrationInstrument::reloadInternalState(src);
    }

    void ZeroSpread::createInstruments(CalibrationInstruments& instrumentList, 
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
            bool foundTenor = IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, TENOR), i, description, emptyStr);	
            bool foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentTable, IDeA_KEY(ZEROSPREAD, ENDDATE), i, endDate, LT::date() );
			
			if(foundTenor && foundEndDate)
				LTQC_THROW(IDeA::MarketException, "Tenor or end Date have to be provided but not both");

            if( foundTenor || foundEndDate)
            {
                const double rate(IDeA::extract<double>(*instrumentTable, IDeA_KEY(ZEROSPREAD, RATE), i));
                if(foundTenor) 
				{
					std::string rollConv, rollRuleConv, calendarEnd, spotRollConv, spotRollRuleConv, spotCalendar, spotDays;
					
					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, ROLLCONVENTION), i, rollConv, emptyStr);
					rollConv = (rollConv != emptyStr) ? rollConv : LTQC::RollConvMethod(RollConvMethod::None).asString().data();

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, ROLLRULECONVENTION), i, rollRuleConv, emptyStr);
					rollRuleConv = (rollRuleConv != emptyStr) ? rollRuleConv : LTQC::RollRuleMethod(RollRuleMethod::None).asString().data();

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, CALENDAR), i, calendarEnd, emptyStr);
					calendarEnd = (calendarEnd != emptyStr) ? calendarEnd : calendar;

 					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, SPOTDAYS), i, spotDays, emptyStr);
					spotDays = (spotDays != emptyStr) ? spotDays : "0C";

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, SPOTROLLCONVENTION), i, spotRollConv, emptyStr);
					spotRollConv = (spotRollConv != emptyStr) ? spotRollConv : LTQC::RollConvMethod(RollConvMethod::None).asString().data();

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, SPOTROLLRULECONVENTION), i, spotRollRuleConv, emptyStr);
					spotRollRuleConv = (spotRollRuleConv != emptyStr) ? spotRollRuleConv : LTQC::RollRuleMethod(RollRuleMethod::None).asString().data();

					IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, SPOTCALENDAR), i, spotCalendar, emptyStr);
					spotCalendar = (spotCalendar != emptyStr) ? spotCalendar : calendarEnd;

					const Date spotDate = LTQC::DateBuilder::dateAdd(Date(valueDate), spotDays, spotCalendar, spotRollConv, spotRollRuleConv, LTQC::RollConvMethod(RollConvMethod::None).asString().data());
					endDate = LTQC::DateBuilder::dateAdd(spotDate, description, calendarEnd, rollConv, rollRuleConv, LTQC::RollConvMethod(RollConvMethod::None).asString().data()).getAsLTdate();
				}                   


                CalibrationInstrumentPtr instrument( new ZeroSpread( foundTenor ? description : endDate.toDDMMMYYYYString(), endDate, rate, valueDate, globalComponentCache) );
                instrumentList.add(instrument);
            }
			else
				LTQC_THROW(IDeA::MarketException, "Neither tenor or end date is provided.");
        }
    }

    void ZeroSpread::updateInstruments(CalibrationInstruments& instrumentList, 
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
            bool foundTenor = IDeA::permissive_extract<std::string>(*instrumentTable, IDeA_KEY(ZEROSPREAD, TENOR), i, description, emptyStr);	
            bool foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentTable, IDeA_KEY(ZEROSPREAD, ENDDATE), i, endDate, LT::date() );
			
            if(foundTenor || foundEndDate)
            {
                const double zeroSpread(IDeA::extract<double>(*instrumentTable, IDeA_KEY(ZEROSPREAD, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(zeroSpread);
                ++(*instrumentIndex);
            }
        }
    }

    double ZeroSpread::getTenor() const
    {
        return m_discountFactor->getArguments().getFlowTime();
    }
    
    
    const double ZeroSpread::computeModelPrice(const BaseModelPtr model) const
    {
		double t  = getTenor();
        double df = m_discountFactor->getValue(*model);

		if(!m_dependentModel)
			initialize(*model);

		double bdf = m_baseDiscountFactor->getValue(*m_dependentModel);
        
        if( t > 0.0 )
            return - ::log(df/bdf)/t;
        else
            return 0.0;
    }

    const double ZeroSpread::computePV(const BaseModelPtr model) const
    {
        return getResidual( model );
    }

    const double ZeroSpread::computeBPV(const BaseModelPtr model) const
    {
        return (::exp(- oneBasisPoint( ) * getTenor() ) -1.0) * m_discountFactor->getValue(*model);
    }

    void ZeroSpread::accumulateGradient(BaseModel const& baseModel, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {   
		double t  = getTenor();
        double df = m_discountFactor->getValue(baseModel);
		
        if( t > 0.0 )
            m_discountFactor->accumulateGradient(baseModel, - multiplier/(df*t), gradientBegin, gradientEnd);
    }

	void ZeroSpread::accumulateGradient(BaseModel const& baseModel,
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
	
	void ZeroSpread::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void ZeroSpread::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void ZeroSpread::update()
    {
        m_discountFactor->update();
    }

    double ZeroSpread::getLastRelevantTime() const 
    {
        return getTenor(); 
    }
	
    double ZeroSpread::getVariableInitialGuess(const double flowTime, const BaseModel* const model) const
	{
		if(!m_dependentModel)
			initialize(*model);
        double bdf = m_baseDiscountFactor->getValue(*m_dependentModel);

		LT_LOG << "initial guess for ZS" << getDescription().string() << ", " << getDescription().string() << std::endl;
		return model->getVariableValueFromSpineDiscountFactor(flowTime, ::exp(- flowTime * getRate() )/bdf, CurveType::_3M());
	}

	double ZeroSpread::calculateRateDerivative(const BaseModelPtr& model) const
	{
		return -getTenor() * m_discountFactor->getValue(*model);
	}
    
	void ZeroSpread::accumulateRateDerivativeGradient(const BaseModel& model,
														  double multiplier,
														  GradientIterator gradientBegin,
														  GradientIterator gradientEnd) const
	{
        double d = -getTenor() * m_discountFactor->getValue(model);
        m_discountFactor->accumulateGradient(model, multiplier * d, gradientBegin, gradientEnd);
	}

	void ZeroSpread::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
        double d = -getTenor() * m_discountFactor->getValue(model);
        m_discountFactor->accumulateGradient(model, multiplier * d, gradientBegin, gradientEnd, curveType);
	}

	double ZeroSpread::computeParRate(const BaseModelPtr& model)
	{ 
		if(!m_dependentModel)
			initialize(*model);
        double bdf = m_baseDiscountFactor->getValue(*m_dependentModel);

        double df = m_discountFactor->getValue(*model);
        double t  = getTenor();
        if( t > 0.0 )
            return - ::log(df/bdf)/t;
        else
            return 0.0;
	}
	
	void ZeroSpread::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                      const BaseModel& model,
									  const double multiplier, 
									  IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
		LTQC_THROW(IDeA::MarketException, "FillRepFlows not implemented for zero rate");
	}
	
	double ZeroSpread::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		LTQuant::GenericDataPtr instrumentData;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(instrumentListData, getKey<ZeroSpread>(), instrumentData);

		if(!instrumentData)
		{
			LTQC_THROW( LTQC::DataQCException, "Cannot find the instrument table \"" << getKeyName<ZeroSpread>().string() << "\"" );
		}

		const size_t nbInstruments(IDeA::numberOfRecords(*instrumentData));
		for(size_t i = 0; i < nbInstruments; ++i)
		{
            std::string description, emptyStr;
            LT::date endDate;

            bool foundTenor = IDeA::permissive_extract<std::string>(*instrumentData, IDeA_KEY(ZEROSPREAD, TENOR), i, description, emptyStr);	
            bool foundEndDate = IDeA::permissive_extract<LT::date>(*instrumentData, IDeA_KEY(ZEROSPREAD, ENDDATE), i, endDate, LT::date() );
			if( (foundTenor && description == this->getDescription().string()) || ( foundEndDate &&  endDate.toDDMMMYYYYString() == this->getDescription().string() ) )
			{
				const double newRate(IDeA::extract<double>(*instrumentData, IDeA_KEY(ZEROSPREAD, RATE)));
				return newRate - this->getRate();					
			}
            else 
            {
                LTQC_THROW( LTQC::DataQCException, "Cannot find an instrument with description \"" << getDescription().string() << "\" in table \"" << getKeyName<ZeroSpread>().string() << "\"" );
            }
		}
        return 0.0;
	}
	
	ostream& ZeroSpread::print(ostream& out) const
    {
        out << "ZS" << getDescription().string();
        return out;
    }

    ICloneLookupPtr ZeroSpread::cloneWithLookup(CloneLookup& lookup) const
    {
        ZeroSpreadPtr clone(new ZeroSpread(*this));
        clone->m_discountFactor = lookup.get(m_discountFactor);
		clone->m_baseDiscountFactor = lookup.get(m_baseDiscountFactor);
        return clone;
    }

	void ZeroSpread::initialize(const BaseModel& baseModel) const
    {
        LT::Str market, asset;
        if( !baseModel.getDependentMarketData() )
        {
            LTQC_THROW( IDeA::ModelException, "FxForwardInstrument: unable to find any dependencies");
        }
        for (size_t i = 1; i < baseModel.getDependentMarketData()->table->rowsGet(); ++i) 
        {
            AssetDomainType adType = AssetDomainType(IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
            LT::Str asset = IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
            if (adType == IDeA::AssetDomainType::IR )
            {
				asset = IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
                market = IDeA::extract<LT::Str>(baseModel.getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1); 
				if(baseModel.hasDependentModel(IRAssetDomain(asset, market)))
				{
					m_dependentModel = baseModel.getDependentModel(IRAssetDomain(asset, market));
					break;
				}
            }
        }
           
        if(!m_dependentModel)
        {
            LTQC_THROW( IDeA::ModelException, "Unable to find dependent yield curve model" );
        }
    }
}
