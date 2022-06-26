/*****************************************************************************
    Todo: - Add header file description


    @Originator

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#ifndef __LIBRARY_MODELS_FUTURESCONVEXITYMODEL_H_INCLUDED
#define __LIBRARY_MODELS_FUTURESCONVEXITYMODEL_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "Gradient.h"
#include "ImpliedVolQuotes.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
    FWD_DECLARE_SMART_PTRS( PriceSupplier )   
    FWD_DECLARE_SMART_PTRS( LiborMarketModel )
    FWD_DECLARE_SMART_PTRS( InstantaneousVol )
    FWD_DECLARE_SMART_PTRS( CorrelationStructure )
}

#include "ModuleStaticData/InternalInterface/IRIndexProperties.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BaseModel )
    FWD_DECLARE_SMART_PTRS( ForwardRate )
    FWD_DECLARE_SMART_PTRS( FuturesConvexityModel )
    FWD_DECLARE_SMART_PTRS( InstantaneousVolCalibrator )
    class GlobalComponentCache;

    /// Represents a model to compute Futures convexity adjustment
    ///  with a LIBOR Market Model
    /// Note: could make a distinct interface.
    ///  - internally, the convexity spreads (i.e difference between
    ///  implied LIBOR and forward rates) are computed for the Tenor
    ///  dates. They are then linearly interpolated. As the convexity
    ///  spreads is convex and incresing as a function of time, this
    ///  should be a reasonable proxy, if all the points of 3M Tenor
    ///  structure are provided up to "last Futures end date"
    ///
    /// TO DO:
    /// - calculate and cache the convexity spreads on Tenor structure 
    /// inside the update function
    /// - add a static constructor that reads instantaneous volatilities,
    /// their parametrisation, and the correlation structure
    class FuturesConvexityModel
    {
    public:
        explicit FuturesConvexityModel(const LT::date valueDate,
                                       const ModuleStaticData::IRIndexPropertiesPtr& irIndexProperties,
                                       const std::vector<LT::date>& futuresStartDates,
                                       const std::vector<LT::date>& futuresEndDates,
                                       const ImpliedVolQuotes& impliedVolQuotes,
                                       const LTQuant::PriceSupplierPtr& priceSupplier,
                                       const LTQuant::GenericDataPtr& lmmTable,
                                       const LTQuant::InstantaneousVolPtr& instantaneousVolatility,
                                       const LTQuant::CorrelationStructurePtr& correlationStructure,
                                       GlobalComponentCache& globalComponentCache);

        // Note:
        // Tenor structure should contain the expiry/maturity dates of the futures
        static FuturesConvexityModelPtr create(const LTQuant::GenericDataPtr& convexityModelTable,
                                               const LTQuant::GenericDataPtr& futuresTable,
                                               const LT::date valueDate,
                                               const ModuleStaticData::IRIndexPropertiesPtr& irIndexProperties,
                                               const LTQuant::PriceSupplierPtr priceSupplier,
                                               GlobalComponentCache& globalComponentCache);

        /// Calibrates the instantaneous volatility of the convexity model to the swap cube
        /// of the price supplier, according to the index properties passed in the constructor
		/// Note: only used the model for DEBUG to see what forward rates look like
        void calibrateToSwaptionCube(/*const BaseModelPtr& model*/);


        // compute the implied libor, i.e. the expectation the forward rate under the risk-neutral measure
        // Note: we compute it under the spot-LIBOR measure
        /*
        double computeImpliedLibor(const BaseModelPtr& model, const double timeToExpiry) const;
        
        
        void accumulateImpliedLiborGradient(const BaseModelPtr& model, 
                                            double multiplier,
                                            GradientIterator gradientBegin,
                                            GradientIterator gradientEnd,
                                            const double timeToExpiry) const;
        */
        
        // Returns the interpolated convexity spread which corresponds to the difference
        //   between the implied LIBOR and the forward rate for futures with the specified
        // time to expiry
        double computeConvexitySpread(const BaseModelPtr& model, const double timeToExpiry) const; 

        void accumulateConvexitySpreadGradient(const BaseModelPtr& model, 
                                               const double multiplier,
                                               const GradientIterator gradientBegin,
                                               const GradientIterator gradientEnd,
                                               const double timeToExpiry) const;

        std::vector<LT::date> getTenorStructure() const;

    private:
        // Not used for now
        // double computeImpliedLiborGivenForwardIndex(const BaseModelPTr& model,
        //                                            const size_t index) const;

        double computeConvexitySpreadGivenForwardIndex(const BaseModelPtr& model,
                                                       const size_t index) const;

        void accumulateConvexitySpreadGradientGivenForwardIndex(const BaseModelPtr& model, 
                                                                const double multiplier,
                                                                const GradientIterator gradientBegin,
                                                                const GradientIterator gradientEnd,
                                                                const size_t index) const;

        double computeSumUpToForwardIndex(const BaseModelPtr& model, 
                                          const size_t upperIndex) const;

        void accumulateSumGradientUpToForwardIndex(const BaseModelPtr& model, 
                                                   const double multiplier,
                                                   const GradientIterator gradientBegin,
                                                   const GradientIterator gradientEnd,
                                                   const size_t upperIndex) const;
        void checkIndex(const size_t index) const;

        LT::date m_valueDate;
        std::vector<LT::date> m_futuresStartDates;
        std::vector<LT::date> m_futuresEndDates;
        //std::vector<date> m_startDates;
        //std::vector<date> m_endDates;
        ImpliedVolQuotes m_impliedVolQuotes;
        //std::string m_lmmModelName;
        LTQuant::LiborMarketModelPtr m_lmm;
        typedef std::vector<ForwardRatePtr> ForwardRateVector;
        ForwardRateVector m_forwardRates;
        double m_timeEpsilon;
        LTQuant::PriceSupplierPtr m_priceSupplier;
        LTQuant::InstantaneousVolPtr m_instantaneousVolatility;
        LTQuant::CorrelationStructurePtr m_correlationStructure;
        std::string m_currencyName;
        std::string m_indexName;
        InstantaneousVolCalibratorPtr m_instantaneousVolCalibrator;
        LTQuant::GenericDataPtr m_lmmTable;
    };
}

#endif //__LIBRARY_MODELS_FUTURESCONVEXITYMODEL_H_INCLUDED