
#ifndef __LIBRARY_PRICERS_FLEXYCF_FXFORWARDINSTRUMENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FXFORWARDINSTRUMENT_H_INCLUDED
#pragma once

//	FlexYCF
#include "CalibrationInstruments.h"
#include "ForwardRate.h"
#include "Gradient.h"
#include "DiscountFactor.h"
#include "ModuleDate/InternalInterface/CompositeDuration.h"
#include "ModuleDate/InternalInterface/FXStaticData.h"
#include "ModuleDate/InternalInterface/FXFunctionalInterface.h"
#include "ModuleDate/InternalInterface/Calendar.h"
// #include "UtilsEnums.h"
#include "src/Enums/FxQuoteType.h"

#include "lt\ptr.h"
#include "BaseModel.h"
#include "DataExtraction.h"
#include "DictYieldCurve.h"
#include "AssetDomain.h"
#include "MarketObject.h"
#include "FactoryEnvelopeH.h"
#include "YieldCurve.h"
#include "FXSpot.h"



// LTQuantLib
#include "Library/DateUtils.h"

namespace IDeA
{
    class AssetDomain;
}

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
    FWD_DECLARE_SMART_PTRS( PriceSupplier )
}


namespace FlexYCF
{
    class GlobalComponentCache;

	class FxForwardInstrument : public FlexYCF::CalibrationInstrument
    {
    public:
        FxForwardInstrument(const std::string& description,
					   const LT::date spotDate,
                       const LT::date deliveryDate,
                       const double rate,
                       const LT::date valueDate,
					   const LT::Str& fxIndex,
					   const IDeA::FxQuoteType quoteType,
					   double scaling,
                       double valueDateRate,
					   const IDeA::FxForwardMktConvention& tradeDetails,
                       GlobalComponentCache& globalComponentCache);
		
		FxForwardInstrument(const std::string& tenorDescription,
					   const LT::date spotDate, 
                       const LT::date deliveryDate,
                       const double rate,
                       const LT::date valueDate,
					   const LT::Str& fxIndex,
					   const IDeA::FxQuoteType quoteType,
					   double scaling,
                       double valueDateRate,
					   const IDeA::FxForwardMktConvention& tradeDetails);

        virtual ~FxForwardInstrument();

        static std::string getName()
        {
            return "FxForward";
        }

		static CalibrationInstrumentPtr create(const LTQuant::GenericData& instrumentParametersTable, 
											   const LT::Ptr<LT::date>& buildDate,
											   const LTQuant::GenericData& curveParametersTable);

        static void createInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);

        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);
        
        double getLastRelevantTime() const;
        double getTenor() const;

        virtual std::string getType() const
        {
            return FxForwardInstrument::getName();
        }

        virtual const double getMarketPrice() const
        {
            if(!m_initialized && IDeA::FxQuoteType::OutRight != m_quoteType)
            {
		        LT_THROW_ERROR("FxForwardInstrument is not initialized and not quoted as outright rate")
            }
            if( IDeA::FxQuoteType::OutRight == m_quoteType )
            {
                return getRate();
            }
           
            const LT::Str& description = getDescription();
            if( description.compareCaseless("T/N") == 0 )
            {
                return m_spotRate;
            }
            if( description.compareCaseless("O/N") == 0 )
            {
               return fxForwardRate(m_valueDateRate,m_domCurrency,m_forCurrency); 
            }
			return fxForwardRate(m_spotRate,m_domCurrency,m_forCurrency);
        }

        virtual const double computeModelPrice(const BaseModelPtr) const;

        /// Computes the PV of the instrument
        virtual const double computePV(const BaseModelPtr model) const;

        /// Computes the basis point value of the instrument
        virtual const double computeBPV(const BaseModelPtr model) const;

        virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier, 
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd);
		virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);

        virtual void update();

		virtual double getVariableInitialGuess(const double flowTime,
											   const BaseModel* const model) const;
		virtual double calculateRateDerivative(const BaseModelPtr& model) const;
		virtual void accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const;
		virtual void accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const;
		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);
		virtual double computeParRate(const BaseModelPtr& model);

		double computeParRate(const BaseModelConstPtr& domModel, const BaseModelConstPtr& forModel, const LT::Str& domModelCcy, const LT::Str& forModelCcy, double spotFX, const LT::Str& domCcy, const LT::Str& forCcy) const;
		double fxForwardRate(double spotRate, const LT::Str& domCcy, const LT::Str& forCcy) const;
        
		LT::Str  fxIndex() const { return m_fxIndex; }
		LT::Str  foreignCurrency() const { return m_forCurrency; }
		LT::Str  domesticCurrency() const { return m_domCurrency; }

		//	IHasRepFlows<IDeA::Index> interface
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,const BaseModel& model, const double multiplier, IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);
		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
        virtual std::ostream& print(std::ostream& out) const;     // Useful for testing

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;
        
        // gets spotRate and foreign model from dependencies
        void initialize(const BaseModel& baseModel) const;

		static LT::date computeFxDeliveryDate(Date valueDate, ModuleDate::FXStaticData const& fxStaticData, const ModuleDate::CompositeDuration& duration)
		{
			Date expiry = static_cast<long>(ModuleDate::getFXExpiryDate(valueDate, fxStaticData, duration));
			Date delivery = computeFxSpotDate(expiry, fxStaticData);
			return delivery.getAsLTdate();
		}
		
		static LT::date computeFxSpotDate(Date valueDate, ModuleDate::FXStaticData const& fxStaticData)
		{
			Date d = static_cast<long>(ModuleDate::getFXSpotDate(valueDate, fxStaticData));
			return d.getAsLTdate();
		}

        static void getSpotRates(const BaseModel& baseModel, std::vector<LT::TablePtr>& fxSpots) 
        {
            const LTQuant::GenericDataPtr& dependencies = baseModel.getDependencies();
            if( !dependencies )
            {
                LT_THROW_ERROR( "FxForward can not find dependencies and spot rate");
            }

            for (size_t i = 1; i < dependencies->table->rowsGet(); ++i) 
		    {
			    LT::Str key = dependencies->table->at(i, IDeA_PARAM(DEPENDENCIES, KEY));
			    LT::TablePtr value = extract(dependencies->table->at(i, IDeA_PARAM(DEPENDENCIES, OBJECT))) || IDeA_PARAM(DEPENDENCIES, OBJECT);

                IDeA::AssetDomainConstPtr adKey(IDeA::AssetDomain::createAssetDomain(key));
               
						
			    if (adKey->getDomainType() == IDeA::AssetDomainType::FXSPOT )
                {
                    fxSpots.push_back(value);
			    }
            }
        }
     
        static void getYieldCurves(const BaseModel& baseModel, std::vector<LT::TablePtr>& yieldCurves) 
        {
            const LTQuant::GenericDataPtr& dependencies = baseModel.getDependencies();
            if( !dependencies )
            {
                LT_THROW_ERROR( "FxForward can not find dependencies and yield curves needed");
            }

            for (size_t i = 1; i < dependencies->table->rowsGet(); ++i) 
		    {
			    LT::Str key = dependencies->table->at(i, IDeA_PARAM(DEPENDENCIES, KEY));
			    LT::TablePtr value = extract(dependencies->table->at(i, IDeA_PARAM(DEPENDENCIES, OBJECT))) || IDeA_PARAM(DEPENDENCIES, OBJECT);

                IDeA::AssetDomainConstPtr adKey(IDeA::AssetDomain::createAssetDomain(key));
               
						
			    if (adKey->getDomainType() == IDeA::AssetDomainType::IR )
                {
                    yieldCurves.push_back(value);
			    }
            }
        }
        
        static double getFxSpotDomToFor(const LT::Str& domCcy, const LT::Str& forCcy, const LT::TablePtr& fxSpotData) 
	    {
            LT::Str domCcyMarket, forCcyMarket;
            getCurrencies(domCcyMarket, forCcyMarket, fxSpotData);

            LT::TablePtr fxSpotRateTable = IDeA::extract<LT::TablePtr>(fxSpotData, IDeA_KEY(FXSPOTMARKET, FXSPOTRATE) );
            double fxSpot = IDeA::extract<double>(fxSpotRateTable, IDeA_KEY(FXSPOTMARKET_FXSPOTRATE, SPOTRATE));
		
		    if (domCcy == forCcyMarket) {
			    if (forCcy != domCcyMarket)
                    LT_THROW_ERROR("In FxSpot, Expected Currency " << forCcy.data() << ". Found " << domCcyMarket.data() );
			    fxSpot = 1.0/fxSpot;
		    } else if (domCcy == domCcyMarket) {
			    if (forCcy != forCcyMarket)
				    LT_THROW_ERROR("In FxSpot, Expected Currency " << forCcy.data() << ". Found " << forCcyMarket.data());
		    } else
			    LT_THROW_ERROR("In FxSpot, Expected Currency " << domCcy.data() << ". Found " << domCcy.data() << " and " << forCcyMarket.data());

		    return fxSpot;
	    }

        static void getCurrencies(LT::Str& domCcy, LT::Str& forCcy, const LT::TablePtr& fxSpotData) 
	    {
            LT::TablePtr curveParams = IDeA::extract<LT::TablePtr>(fxSpotData, IDeA_KEY(FXSPOTMARKET, CURVEPARAMETERS) );
            LT::Str fxPairStr = IDeA::extract<LT::Str>(curveParams, IDeA_KEY(FXSPOTMARKET_CURVEPARAMETERS, FXPAIR));
	
            IDeA::FxSpot::getCurrencies(domCcy, forCcy, fxPairStr);
	    }
        
        static double getFxSpotDomToFor(const LT::Str& domCcy, const LT::Str& forCcy, const std::vector<LT::TablePtr>& fxSpotData, const std::vector<LT::TablePtr>& yieldCurves)
	    {
		    std::vector<LT::TablePtr> fxSpotCrosses;
		    findFxCrosses(fxSpotData, domCcy, forCcy, fxSpotCrosses);
		    if( fxSpotCrosses.size() == 1 )
		    {
			    return getFxSpotDomToFor(domCcy, forCcy, fxSpotCrosses[0]);
		    }
		    if( fxSpotCrosses.size() == 2 )
		    {	
			    return getCrossFxSpot(domCcy, forCcy, fxSpotCrosses[0], fxSpotCrosses[1], yieldCurves);
		    }
		    LT_THROW_ERROR("Invalid FX Pairs passed");
	    }

        static void findFxCrosses(const std::vector<LT::TablePtr>& fxSpots, const LT::Str& domCcy, const LT::Str& forCcy, std::vector<LT::TablePtr>& fxSpotCrosses) 
	    {
		    LT::Str ccy1, ccy2;
		    for(size_t i = 0; i < fxSpots.size(); ++i)
		    {
			    getCurrencies(ccy1, ccy2, fxSpots[i]);
			    if( (domCcy.compareCaseless(ccy1) == 0 && forCcy.compareCaseless(ccy2) == 0) ||  (domCcy.compareCaseless(ccy2) == 0 && forCcy.compareCaseless(ccy1) == 0) )
			    {
				    fxSpotCrosses.push_back(fxSpots[i]);
				    return;
			    }
		    }

		    std::vector<LT::TablePtr> fxSpotDomCcy, fxSpotForCcy;
		    for(size_t i = 0; i < fxSpots.size(); ++i)
		    {
			    getCurrencies(ccy1, ccy2, fxSpots[i]);
			    if( (domCcy.compareCaseless(ccy1) == 0 || domCcy.compareCaseless(ccy2) == 0) )
			    {
				    fxSpotDomCcy.push_back(fxSpots[i]);
			    }
			    if( (forCcy.compareCaseless(ccy1) == 0 || forCcy.compareCaseless(ccy2) == 0) )
			    {
				    fxSpotForCcy.push_back(fxSpots[i]);
			    }
		    }

		    if( fxSpotDomCcy.size() == 0 || fxSpotDomCcy.size() == 0 )
		    {
			    return;
		    }

		    for(size_t i = 0; i < fxSpotDomCcy.size(); ++i)
		    {	
			    LT::Str ccy1, ccy2;
			    getCurrencies(ccy1, ccy2, fxSpotDomCcy[i]);
			    if(domCcy.compareCaseless(ccy1) == 0)
			    {
				    ccy1 = ccy2;
			    }
			
			    for(size_t j = 0; j < fxSpotForCcy.size(); ++j)
			    {
				    LT::Str ccy3, ccy4;
				    getCurrencies(ccy3, ccy4, fxSpotForCcy[i]);
				    if(forCcy.compareCaseless(ccy3) == 0)
				    {
					    ccy3 = ccy4;
				    }
				    if(ccy1.compareCaseless(ccy3) == 0)
				    {
					    fxSpotCrosses.push_back(fxSpotDomCcy[i]);
					    fxSpotCrosses.push_back(fxSpotForCcy[j]);
					    return;
				    }
			    }
		    }
	    }
    
        static LT::Str getCrossCurrency(const LT::TablePtr& fxSpotData1, const LT::TablePtr& fxSpotData2)
	    {
            LT::Str crossCcy;
            LT::Str domCcy1, forCcy1, domCcy2, forCcy2;
		    getCurrencies(domCcy1, forCcy1, fxSpotData1);
		    getCurrencies(domCcy2, forCcy2, fxSpotData2);
		    if( domCcy1 == domCcy2 || domCcy1 == forCcy2 )
		    {
			    crossCcy = domCcy1;
		    }
		    if( forCcy1 == domCcy2 || forCcy1 == forCcy2 )
		    {
			    crossCcy =  forCcy1;
		    }
		    return crossCcy;
	    }
         
        static double computeFxSpotFromTodayFxRate(double flowTime, const LT::TablePtr& ycCcy1, const LT::Str& ccy1, const LT::TablePtr& ycCcy2, const LT::Str& ccy2, double todayFx, const LT::Str& domCcy, const LT::Str& forCcy)
	    {
            FlexYCF::BaseModelPtr model1 = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(ycCcy1)->getYieldCurve()->getModel();
            FlexYCF::BaseModelPtr model2 = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(ycCcy2)->getYieldCurve()->getModel();
        
		    double df1 = model1->getDiscountFactor(flowTime);
		    double df2 = model2->getDiscountFactor(flowTime);
		    if( domCcy.compareCaseless(ccy1) == 0)
		    {
			    return todayFx * df2 / df1;
		    }
		    if( domCcy.compareCaseless(ccy2) == 0)
		    {
			    return todayFx * df1 / df2;
		    }
		    LT_THROW_ERROR("Cross FX rates not implemented yet ");
	    }
        
        static double computeTodayFxRateFromSpot(double flowTime, const LT::TablePtr& ycCcy1, const LTQC::Currency& ccy1, const LT::TablePtr& ycCcy2, const LTQC::Currency& ccy2, const LT::TablePtr& fxSpot)
	    {
            FlexYCF::BaseModelPtr model1 = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(ycCcy1)->getYieldCurve()->getModel();
            FlexYCF::BaseModelPtr model2 = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(ycCcy2)->getYieldCurve()->getModel();
        
		    double df1 = model1->getDiscountFactor(flowTime);
		    double df2 = model2->getDiscountFactor(flowTime);
		    double spot = getFxSpotDomToFor(ccy1, ccy2, fxSpot);
		    return spot * df1 / df2;
	    }

        static double getCrossFxSpot(const LT::Str& domCcy, const LT::Str& forCcy, const LT::TablePtr& fxSpotData1, const LT::TablePtr& fxSpotData2, const std::vector<LT::TablePtr>& yieldCurves)
	    {
		    LT::Str domCcy1, forCcy1;
		    getCurrencies(domCcy1, forCcy1, fxSpotData1);

		    if ((domCcy1 == domCcy && forCcy1 == forCcy) || (domCcy1 == forCcy && forCcy1 == domCcy)) 
		    {
			    return getFxSpotDomToFor(domCcy, forCcy, fxSpotData1);
		    } 
		    else 
		    {
			    LT::Str domCcy2, forCcy2;
			    getCurrencies(domCcy2, forCcy2, fxSpotData2);

			    if ((domCcy2 == domCcy && forCcy2 == forCcy) || (domCcy2 == forCcy && forCcy2 == domCcy)) 
			    {
				    return getFxSpotDomToFor(domCcy, forCcy, fxSpotData2);
			    }	 

			    LT::Str cross = getCrossCurrency(fxSpotData1, fxSpotData2);
			    if( cross.empty() )
			    {
				    LT_THROW_ERROR("Can not find cross-currency");
			    }
			
			    LT::TablePtr domYC, forYC, crossYC;
			    for(size_t i = 0; i < yieldCurves.size(); ++i)
			    {
                    IDeA::AssetDomainConstPtr ad = IDeA::MarketObject::getAssetDomain(*(yieldCurves[i]));
				    LT::Str ccy  = ad->primaryDomain();
				    if(domCcy.compareCaseless(ccy) == 0)
				    {
					    domYC = yieldCurves[i];
				    }
				    if(forCcy.compareCaseless(ccy) == 0)
				    {
					    forYC = yieldCurves[i];
				    }
				    if(cross.compareCaseless(ccy) == 0)
				    {
					    crossYC = yieldCurves[i];
				    }
			    }
			    if( !domYC || !forYC || !crossYC )
			    {
				    LT_THROW_ERROR("Can not find yield curve for fx cross");
			    }

			    if( domCcy == domCcy1 || domCcy == forCcy1)
			    {
				    double flowTime1 = fxSpotFlowTime(domCcy, cross, fxSpotData1, domYC, crossYC);
				    double fxToday1  = computeTodayFxRateFromSpot(flowTime1, domYC, domCcy, crossYC, cross, fxSpotData1);
				    double flowTime2 = fxSpotFlowTime(cross, forCcy, fxSpotData2, crossYC, forYC);
				    double fxToday2  = computeTodayFxRateFromSpot(flowTime2, crossYC, cross, forYC, forCcy, fxSpotData2);
				    double flowTime3 = fxSpotFlowTime(domCcy, forCcy, LT::TablePtr(), domYC, forYC);
				    double crossFx   = computeFxSpotFromTodayFxRate(flowTime3, domYC, domCcy, forYC, forCcy, fxToday1 * fxToday2, domCcy, forCcy);
				
				    return crossFx;
			    }
			    else
			    {
				    double flowTime1 = fxSpotFlowTime(domCcy, cross, fxSpotData2, domYC, crossYC);
				    double fxToday1  = computeTodayFxRateFromSpot(flowTime1, domYC, domCcy, crossYC, cross, fxSpotData2);
				    double flowTime2 = fxSpotFlowTime(cross, forCcy, fxSpotData1, crossYC, forYC);
				    double fxToday2  = computeTodayFxRateFromSpot(flowTime2, crossYC, cross, forYC, forCcy, fxSpotData1);
				    double flowTime3 = fxSpotFlowTime(domCcy, forCcy, LT::TablePtr(), domYC, forYC);
				    double crossFx   = computeFxSpotFromTodayFxRate(flowTime3, domYC, domCcy, forYC, forCcy, fxToday1 * fxToday2, domCcy, forCcy);
				
				    return crossFx;
			    }
		    }
	    }
        
       

        static Date fxSpotDate(const LT::Str& domCcy, const LT::Str& forCcy, const LT::TablePtr& fxSpotData, const LT::TablePtr& domYC, const LT::TablePtr& forYC, LT::date& valueDate) 
	    {
		    std::string ccypair= domCcy.string() + forCcy.string();
            ModuleDate::FXStaticData fxStaticData;
	        bool hasDefaults = ModuleDate::FXStaticData::tryGet(ccypair, fxStaticData);
		
		    LT::Str  spotDaysStr = fxSpotData ? fxSpotData->atXpath("Curve Parameters/Spot Days") : LT::Str();
		    if ( spotDaysStr.empty() ) 
		    {
			    if (!hasDefaults)
			    {
				    LT_THROW_ERROR("Unable to obtain defaults from static data. Please enter spot days");
			    }
		    }
		    else
		    {
                // Setting all the spot days to be the explicit one. This inhibits the behaviour of finding earlier spot dates for non-USD pairs.
                LTQC::Tenor spotDays = LTQC::Tenor(spotDaysStr);
			    fxStaticData.setSpotDays(spotDays.asDays());
		    }

		    LT::Str cal1, cal2;
		    if(!hasDefaults)
		    {
			    const LT::TablePtr curveParamsTable1(IDeA::extract<LT::TablePtr>(*domYC, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
			    bool found1(IDeA::permissive_extract<LT::Str>(*curveParamsTable1, IDeA_KEY(YC_CURVEPARAMETERS, HOLIDAYCALENDAR), cal1, cal1));
			    if(!found1)
			    {
				    LT_THROW_ERROR("Unable to obtain defaults from static data. Please enter money calendar");
			    }	
			    const LT::TablePtr curveParamsTable2(IDeA::extract<LT::TablePtr>(*forYC, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
			    bool found2(IDeA::permissive_extract<LT::Str>(*curveParamsTable2, IDeA_KEY(YC_CURVEPARAMETERS, HOLIDAYCALENDAR), cal2, cal2));
			    if(!found2)
			    {
				    LT_THROW_ERROR("Unable to obtain defaults from static data. Please enter main calendar");
			    }

		        fxStaticData.setExplicit(cal1.cStr(), cal2.cStr());
		    }

		    const LT::TablePtr detailsTable(IDeA::extract<LT::TablePtr>(*domYC, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		    valueDate = IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE));
	       
		    Date spotDate = static_cast<long>(ModuleDate::getFXSpotDate(Date(valueDate), fxStaticData));
            return spotDate;
		   
	    }
        
        static double fxSpotFlowTime(const LT::Str& domCcy, const LT::Str& forCcy, const LT::TablePtr& fxSpotData, const LT::TablePtr& domYC, const LT::TablePtr& forYC)
        {
            LT::date valueDate;
            Date spotDate = fxSpotDate(domCcy, forCcy, fxSpotData, domYC, forYC, valueDate); 
            return ModuleDate::getYearsBetween(valueDate,spotDate.getAsLTdate());
        }
        
        virtual void finishCalibration(const BaseModelPtr model);

        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src);

    protected:
        FxForwardInstrument(const FxForwardInstrument& original, CloneLookup& lookup);

    private:
        FxForwardInstrument(const FxForwardInstrument&); // deliberately disabled as won't clone properly

		DiscountFactorPtr						m_deliveryDiscountFactor;
		DiscountFactorPtr						m_spotDiscountFactor;
		LT::Str						            m_fxIndex;
		LT::Str								    m_domCurrency;
        LT::Str					  		    	m_forCurrency;
		IDeA::FxQuoteType						m_quoteType;
		double									m_scaling;

        // valid after initialization
        mutable bool                            m_initialized;
        mutable double                          m_spotRate;             // fx spot rate
        mutable double                          m_valueDateRate;        // fxSpotRate - O/N - T/N (spreadDiscount) or fxSpotRate + O/N + T/N (spreadPremium)
        mutable BaseModelConstPtr               m_dependentModel;
        mutable LT::Str                         m_dependentModelCcy;
    };

    DECLARE_SMART_PTRS( FxForwardInstrument )

}
#endif //__LIBRARY_PRICERS_FLEXYCF_FXFORWARDINSTRUMENT_H_INCLUDED