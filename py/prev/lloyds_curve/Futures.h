/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FUTURES_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FUTURES_H_INCLUDED
#pragma once

#include "CalibrationInstruments.h"
#include "ForwardRate.h"
#include "Gradient.h"

namespace IDeA
{
    class AssetDomain;
}

namespace LTQuant
{
    FWD_DECLARE_SMART_PTRS( PriceSupplier )
}

namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )
}

namespace FlexYCF
{
    class GlobalComponentCache;

    FWD_DECLARE_SMART_PTRS( FuturesConvexityModel )

    /// Futures represents a futures instrument that
    /// can be used to calibrate a model.
        ///
    /// Note: quoted price is divided by 100 (e.g. 0.9742)
    /// The pricing formulae are as follows:
    /// - market price = quoted price
    /// - model price = 1.0 - futures rate
    ///     where futures rate = forward rate + convexity adj.
    ///
    /// TO DO: 
    /// - in the static constructor, check a flag indicating whether
    /// to build a futures convexity model and pass it to each Futures ctor
    /// - remove/move the m_convexityAdjustment double to a 
    /// ConstantFuturesConvexityModel?
    class Futures : public CalibrationInstrument
    {
    public:
        Futures(const std::string dateDescription,
                const LT::date valueDate,
                const LT::date fixingDate,
                const LT::date startDate, 
                const LT::date endDate, 
                const double quotedPrice, 
                const double convexityAdjustment,
                const ModuleDate::DayCounterConstPtr basis,
                const double contractSize,
                GlobalComponentCache& globalComponentCache,
                const FuturesConvexityModelPtr convexityModel);

        Futures(const std::string dateDescription,
                const LT::date valueDate,
                const LT::date fixingDate,
                const LT::date startDate, 
                const LT::date endDate, 
                const double quotedPrice, 
                const double convexityAdjustment,
                const ModuleDate::DayCounterConstPtr basis,
                const double contractSize);

        static std::string getName()
        {
            return "Futures";
        }

		static CalibrationInstrumentPtr create(const LTQuant::GenericData& instrumentParametersTable,
											   const LT::Ptr<LT::date>& buildDate,
											   const LTQuant::GenericData& curveParametersTable,
											   const LTQuant::GenericDataPtr&);

        static void createInstruments(CalibrationInstruments& instruments, 
                                      LTQuant::GenericDataPtr instrumentTable,
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr priceSupplier);
    
        static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);

        FuturesConvexityModelPtr getConvexityModel() const
        { 
            return m_convexityModel;  
        }
		void setConvexity(double convexity) 
		{
			m_convexityAdjustment = convexity;
		}
		
        virtual std::string getType() const
        {
            return Futures::getName();
        }

        /// Returns 100 - the Futures quoted price
        virtual const double getMarketPrice() const;

        ///  If the Futures has a convexity model, computes the implied LIBOR 
        /// (i.e. the rate such that 100 - impliedLIBOR is the quoted price)
        /// Otherwise juste computes the (unadjusted) forward rate
        virtual const double computeModelPrice(const BaseModelPtr) const;

        /// Computes the PV of the instrument:
        /// In the futures case, we actually compute here its value, which is not strictly speaking a present value.
        virtual const double computePV(const BaseModelPtr model) const;

        /// Computes the basis point value of the instrument
        virtual const double computeBPV(const BaseModelPtr model) const;
        
        /// Accumulates the gradient of the corresponding implied LIBOR or the forward rate,
        /// depending on whether the Futures has a convexity model or not, resp.
        virtual void accumulateGradient(BaseModel const& baseModel, 
										double multiplier,
										GradientIterator gradientBegin,
										GradientIterator gradientEnd);
		virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);
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
        virtual void update();
        
        virtual double getLastRelevantTime() const;
		
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
		virtual double computeParRate(const BaseModelPtr& model);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
							      const double multiplier, 
							      IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);
		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

        // double computeImpliedLibor(const BaseModelPtr& model);

        /// Returns the convexity adjustment (calculated by a convexity model
		///	if there is one, just a constant value set from a table on construction
		///	otherwise)
        double getConvexityAdjustment(const BaseModelPtr& model) const;
        
		inline double getConvexityAdjustment() const
		{
			return m_convexityAdjustment;
		}

        virtual void finishCalibration(const BaseModelPtr model);

        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr& src);
    protected:
        Futures(Futures const& original, CloneLookup& lookup);

    private:
        Futures(Futures const&); // deliberately disabled as won't clone properly

        ForwardRatePtr				m_forwardRate;
        double						m_convexityAdjustment;        
        FuturesConvexityModelPtr	m_convexityModel;
        double						m_timeToExpiry;      // useful for convexity calculations
        double                      m_contractSize;
    };  // Futures

    DECLARE_SMART_PTRS( Futures )

}   // FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FUTURES_H_INCLUDED