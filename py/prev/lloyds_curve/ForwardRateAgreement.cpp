/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

#include "ForwardRateAgreement.h"
#include "BaseModel.h"
#include "Data/GenericData.h"
#include "Instruments/Index.h"
#include "Data/MarketData/YieldCurveCreator.h"
#include "ForwardRate.h"
#include "GlobalComponentCache.h"
#include "Pricers/PriceSupplier.h"
#include "ModuleDate/InternalInterface/Utils.h"
#include "DataExtraction.h"
#include "DateUtils.h"
#include "FlexYcfUtils.h"
#include "ModuleDate/InternalInterface/CalendarFactory.h"
#include "FlexYCFCloneLookup.h"
#include "RepFlowsData.h"

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

using namespace LTQC;
using namespace std;

namespace {
    void extractStartAndEndTenorInMonths( const string& description, int& startTenorInMonths, int& endTenorInMonths )
    {
        std::vector< LT::Str > tokens;
        LTQC::split(tokens, LT::trimWhitespace(description), "x");
        const LT::Str errorMsg( "An invalid FRA reference was provided: please provide it in the exact format 'nxm' where n and m are positive integers such that n < m, and m - n defines the tenor in months of the underlying IBOR rate." );
        if( tokens.size( ) != 2 ) 
        {
            LTQC_THROW( IDeA::MarketException, errorMsg.data( ) );
        }
        startTenorInMonths = atoi( tokens[ 0 ].data() );
        endTenorInMonths = atoi( tokens[ 1 ].data() );
        if( startTenorInMonths < 0 || endTenorInMonths <= startTenorInMonths )
        {
            LTQC_THROW( IDeA::MarketException, errorMsg.data( ) );
        }
        if( !startTenorInMonths )
        {
            LTQC_THROW( IDeA::MarketException, "Spot starting FRAs not supported." );
        }
    }
}

namespace FlexYCF
{
	template<>
	const IDeA::DictionaryKey& getKey<ForwardRateAgreement>()
	{
		return IDeA_KEY(YC_INSTRUMENTLIST, FRA);
	}

    using namespace LTQuant;
    using namespace ModuleDate;

    ForwardRateAgreement::ForwardRateAgreement(const std::string description,
                                               const LT::date valueDate,
                                               const LT::date fixingDate,
                                               const LT::date startDate, 
                                               const LT::date endDate,
                                               const double quote,
                                               const LTQC::Tenor rateTenor,
                                               const DayCounterConstPtr basis,
                                               GlobalComponentCache& globalComponentCache):
		CalibrationInstrument(quote, getKeyName<ForwardRateAgreement>(), description, fixingDate, startDate, endDate),
        m_forwardRate(globalComponentCache.get( ForwardRate::Arguments( valueDate,
																		fixingDate,
                                                                        max(valueDate, startDate), // N.B. why check? Should be expired
                                                                        endDate,
                                                                        rateTenor.asString().data(),
                                                                        basis,
                                                                        globalComponentCache))),
        m_curveType( CurveType::getFromDescription( rateTenor.asString( ) ) )
    {
    }

    ForwardRateAgreement::ForwardRateAgreement(const std::string description,
                                               const LT::date valueDate,
                                               const LT::date fixingDate,
                                               const LT::date startDate, 
                                               const LT::date endDate,
                                               const double quote,
                                               const LTQC::Tenor rateTenor,
                                               const DayCounterConstPtr basis):
        CalibrationInstrument(quote, getKeyName<ForwardRateAgreement>(), description, fixingDate, startDate, endDate),
        m_forwardRate( ForwardRate::create( ForwardRate::Arguments( valueDate,
											                        fixingDate,
                                                                    max(valueDate, startDate),
                                                                    endDate,
                                                                    rateTenor.asString().data(),
                                                                    basis)) ),
       m_curveType( CurveType::getFromDescription( rateTenor.asString( ) ) )
    {
    }

	CalibrationInstrumentPtr ForwardRateAgreement::create(const LTQuant::GenericData& instrumentParametersTable, 
											              const LT::Ptr<LT::date>& buildDate,
											              const LTQuant::GenericData& curveParametersTable,
														  const LTQuant::GenericDataPtr&)
	{
        if( !buildDate )
        {
            LTQC_THROW( IDeA::MarketException, "To create a forward rate agreement instrument, one needs to provide a build date" );
        }
        const LT::date valueDate( *buildDate );
        const std::string description( IDeA::extract< std::string >( instrumentParametersTable, IDeA_KEY(FRA, DESCRIPTION) ) );
        IDeA::IRCurveMktConventionPtr conventions( createIRCurveMktConventions( curveParametersTable ) );
		IDeA::FRAMktConvention& fraDetails = conventions->m_fra;
        DayCounterPtr fraBasis( LTQC::DayCount::create( fraDetails.m_depositRateMktConvention.m_dcm ) );
        CalendarPtr fraCalendar( CalendarFactory::create( fraDetails.m_depositRateMktConvention.m_accrualValueCalendar.string( ) ) );
        LT::date startDate, endDate;
        LTQC::Tenor rateTenor;
        if( !IDeA::permissive_extract< LT::date >( instrumentParametersTable, IDeA_KEY(FRA, STARTDATE), startDate ) 
            || !IDeA::permissive_extract< LT::date >( instrumentParametersTable, IDeA_KEY(FRA, ENDDATE), endDate) )
        {
            int startTenorInMonths, endTenorInMonths;
            extractStartAndEndTenorInMonths( description, startTenorInMonths, endTenorInMonths );
            const LT::date spotDate = IDeA::TradeConventionCalcH::getSpotDate( Date( valueDate ), fraDetails.m_depositRateMktConvention.m_fixingCalendar, fraDetails.m_depositRateMktConvention.m_accrualValueCalendar, fraDetails.m_depositRateMktConvention.m_spotDays ).getAsLTdate( );
            const LTQC::Tenor startTenor( 0, 0, 0, startTenorInMonths, 0 );
            startDate = startTenorInMonths == 0 ? spotDate : addDatePeriod( spotDate, startTenor.asString( ).data( ), fraCalendar );
            rateTenor = LTQC::Tenor( 0, 0, 0, endTenorInMonths - startTenorInMonths, 0 );
            endDate = addDatePeriod( startDate, rateTenor.asString( ).data( ), fraCalendar );
        }
        else
        {
            const double rateYearFraction = fraBasis->getDaysOverBasis( startDate, endDate );
            rateTenor = LTQC::Tenor( CurveType::getFromYearFraction( rateYearFraction )->getDescription( ) );
        }
        
        const LT::date fixingDate = IDeA::IRDepositRateCalcH::calculateFixingDate( Date( startDate ), fraDetails.m_depositRateMktConvention.m_spotDays, fraDetails.m_depositRateMktConvention.m_fixingCalendar ).getAsLTdate( );
		return CalibrationInstrumentPtr( new ForwardRateAgreement(description,
                                                                  valueDate,
                                                                  fixingDate,
                                                                  startDate, 
                                                                  endDate,
                                                                  0.0,
                                                                  rateTenor,
                                                                  fraBasis)
                                        );
	}

    void ForwardRateAgreement::finishCalibration(const BaseModelPtr model)
    {
        CalibrationInstrument::finishCalibration(model);
        m_forwardRate.reset();
    }

    void ForwardRateAgreement::reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src)
    {
        const ForwardRateAgreementPtr ourTypeSrc=std::tr1::static_pointer_cast<ForwardRateAgreement>(src);
        if(!ourTypeSrc)
        {
            LTQC_THROW(IDeA::SystemException, "Can't reaload state from different type of instrument");
        }

        m_forwardRate=ourTypeSrc->m_forwardRate;

        CalibrationInstrument::reloadInternalState(src);
    }
    void ForwardRateAgreement::createInstruments(CalibrationInstruments& instruments, 
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
		const IDeA::FRAMktConvention& fraDetails = conventions->m_fra;
        
		DayCounterPtr fraBasis( LTQC::DayCount::create( fraDetails.m_depositRateMktConvention.m_dcm ) );
		CalendarPtr fraCalendar( CalendarFactory::create( fraDetails.m_depositRateMktConvention.m_accrualValueCalendar.string( ) ) );
        LT::date startDate, endDate;
        for(size_t i(0); i < instrumentTable->numItems() - 1; ++i)
        {
			std::string description(IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(FRA, DESCRIPTION), i));

			if(!description.empty())
            {
                LTQC::Tenor rateTenor;
                const double rate( IDeA::extract<double>(*instrumentTable, IDeA_KEY(FRA, RATE), i) );
                if( !IDeA::permissive_extract< LT::date >( *instrumentTable, IDeA_KEY(FRA, STARTDATE), i, startDate ) 
                   || !IDeA::permissive_extract< LT::date >( *instrumentTable, IDeA_KEY(FRA, ENDDATE), i, endDate) )
                {
                    int startTenorInMonths, endTenorInMonths;
                    extractStartAndEndTenorInMonths( description, startTenorInMonths, endTenorInMonths );
                    const LT::date spotDate = IDeA::TradeConventionCalcH::getSpotDate( Date( valueDate ), fraDetails.m_depositRateMktConvention.m_fixingCalendar, fraDetails.m_depositRateMktConvention.m_accrualValueCalendar, fraDetails.m_depositRateMktConvention.m_spotDays ).getAsLTdate( );
                    const LTQC::Tenor startTenor( 0, 0, 0, startTenorInMonths, 0 );
                    startDate = startTenorInMonths == 0 ? spotDate : addDatePeriod( spotDate, startTenor.asString( ).data( ), fraCalendar );
                    rateTenor = LTQC::Tenor( 0, 0, 0, endTenorInMonths - startTenorInMonths, 0 );
                    endDate = addDatePeriod( startDate, rateTenor.asString( ).data( ), fraCalendar );
                }
                else
                {
                    const double rateYearFraction = fraBasis->getDaysOverBasis( startDate, endDate );
                    rateTenor = LTQC::Tenor( CurveType::getFromYearFraction( rateYearFraction )->getDescription( ) );
                }
				const LT::date fixingDate = IDeA::IRDepositRateCalcH::calculateFixingDate(Date(startDate), fraDetails.m_depositRateMktConvention.m_spotDays, fraDetails.m_depositRateMktConvention.m_fixingCalendar).getAsLTdate();
                CalibrationInstrumentPtr instrument( new ForwardRateAgreement(description,
                                                                              valueDate,
                                                                              fixingDate,
                                                                              startDate, 
                                                                              endDate,
                                                                              rate,
                                                                              rateTenor,
                                                                              fraBasis)
                                                   );
                instruments.add(instrument);
            }
        }
    }

    void ForwardRateAgreement::updateInstruments(CalibrationInstruments& instrumentList, 
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
			const std::string description( IDeA::extract<std::string>(*instrumentTable, IDeA_KEY(FRA, DESCRIPTION), i) );
            if(!description.empty())
            {
				const double quote(IDeA::extract<double>(*instrumentTable, IDeA_KEY(FRA, RATE), i));
                
                instrumentList[*instrumentIndex]->setRate(quote);
                ++(*instrumentIndex);
            }
        }
    }

    const double ForwardRateAgreement::getMarketPrice() const
    {
        return getRate();
    }

    const double ForwardRateAgreement::computeModelPrice(const BaseModelPtr model) const
    {
        return m_forwardRate->getValue( *model );
    }

    const double ForwardRateAgreement::computePV(const BaseModelPtr model) const
    {
        const double dividor = 1.0 + m_forwardRate->getCoverage( ) * m_forwardRate->getValue( *model );
        return m_forwardRate->getCoverage( ) * getResidual( model ) * model->getDiscountFactor( m_forwardRate->getTimeToExpiry( ) ) / dividor;
    }

    const double ForwardRateAgreement::computeBPV(const BaseModelPtr model) const
    {
        const double dividor = 1.0 + m_forwardRate->getCoverage( ) * m_forwardRate->getValue( *model );
        return - m_forwardRate->getCoverage( ) * oneBasisPoint( ) * model->getDiscountFactor( m_forwardRate->getTimeToExpiry( ) ) / dividor;
    }

    void ForwardRateAgreement::accumulateGradient(BaseModel const& model, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd)
    {
        m_forwardRate->accumulateGradient(model, multiplier, gradientBegin, gradientEnd);
    }

	void ForwardRateAgreement::accumulateGradient(BaseModel const& baseModel,
                                     double multiplier,
									 GradientIterator gradientBegin,
                                     GradientIterator gradientEnd,
									 const CurveTypeConstPtr& curveType)
	{
		m_forwardRate->accumulateGradient(baseModel, multiplier, gradientBegin, gradientEnd, curveType);
	}
	
	void ForwardRateAgreement::accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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
    
	void ForwardRateAgreement::accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
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

    void ForwardRateAgreement::update()
    {
        m_forwardRate->update();
    }    

	double ForwardRateAgreement::getVariableInitialGuess(const double flowTime,
											const BaseModel* const model) const
	{
		LT_LOG << "initial guess for FRA " << m_curveType->getDescription( ) << endl;
        const double rateTenor = m_forwardRate->getTenor( );
        const double coverage = m_forwardRate->getCoverage( );
        CurveTypeConstPtr curveType( CurveType::getFromYearFraction( rateTenor ) );
		return model->getVariableValueFromSpineDiscountFactor(flowTime, 
			model->getSpineDiscountFactor(flowTime - rateTenor, curveType) / (1.0 + coverage * getRate()),
			curveType);
	}

	double ForwardRateAgreement::calculateRateDerivative(const BaseModelPtr&) const
	{
		return -1.0;
	}
	
	void ForwardRateAgreement::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const
	{
		//	Do nothing
	}

	void ForwardRateAgreement::accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const
	{
		//	Do nothing
	}

	double ForwardRateAgreement::computeParRate(const BaseModelPtr& model)
	{
		return m_forwardRate->getValue(*model);
	}
	
	void ForwardRateAgreement::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                            const BaseModel& model,
							                const double multiplier, 
							                IDeA::RepFlowsData<IDeA::Index>& indexRepFlows)
	{
        const double refRate = m_forwardRate->getValue( model );
        const double dividor = 1.0 + m_forwardRate->getCoverage( ) * refRate;
        const double modifiedMultiplier = m_forwardRate->getCoverage( ) * ( 1.0 + m_forwardRate->getCoverage( ) * getRate( ) ) / dividor / dividor * model.getDiscountFactor( m_forwardRate->getTimeToExpiry( ) ) * multiplier;
		m_forwardRate->fillRepFlows(assetDomain, model, modifiedMultiplier, indexRepFlows);
	}

    void ForwardRateAgreement::fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                            const BaseModel& model,
							                const double multiplier, 
							                IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
	{
        const double refRate = m_forwardRate->getValue( model );
        const double dividor = 1.0 + m_forwardRate->getCoverage( ) * refRate;
        const double fundingFlow =  m_forwardRate->getCoverage( ) * ( refRate - getRate( ) ) / dividor;
        fundingRepFlows.addRepFlow( IDeA::Funding::Key(assetDomain, getStartDate( )), multiplier * fundingFlow );
	}

	double ForwardRateAgreement::getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const
	{
		return doGetDifferenceWithNewRate(instrumentListData, getKey<ForwardRateAgreement>(), IDeA_KEY(FRA, DESCRIPTION), IDeA_KEY(FRA, RATE));
	}

    ostream& ForwardRateAgreement::print(ostream& out) const
    {
        out << ForwardRateAgreement::getName() << getDescription().string();
        return out;
    }

    /**
        @brief Clone this futures instrument.

        Uses a lookup to maintain directed graph relationships.

        @param lookup A lookup of previously created clones.
        @return       The clone of this instance.
    */
    ICloneLookupPtr ForwardRateAgreement::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new ForwardRateAgreement(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    ForwardRateAgreement::ForwardRateAgreement(ForwardRateAgreement const& original, CloneLookup& lookup) :
        CalibrationInstrument(original),
        m_forwardRate(lookup.get(original.m_forwardRate)),
        m_curveType(original.m_curveType)
    {
    }
} // FlexYCF
