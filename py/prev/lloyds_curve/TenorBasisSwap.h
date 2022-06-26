/*****************************************************************************

	TenorBasisSwap    
    
	Represents a LTQC::Tenor Basis Swap instrument.

	@Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_TENORBASISSWAP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_TENORBASISSWAP_H_INCLUDED
#pragma once

//	FlexYCR
#include "CalibrationInstruments.h"
#include "AllComponentsAndCaches.h"
#include "StubUtils.h"

//	QuantLib
#include "ModuleDate/InternalInterface/ScheduleGenerator.h"

//	IDeA
#include "IDeA\src\market\MarketConvention.h"

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

    /**
    * This is a helper class which purpose is to join the interest rate curve parameters and the market conventions
    */
    class TenorBasisSwapDetails
	{
		public:
		struct FloatLeg {
            IDeA::DepositRateMktConvention	m_depRateMktConvention;
            LTQC::Tenor						m_frequency;		
            LTQC::DayCountMethod			m_accrualBasis;
			LT::Str						    m_rollConvention;
			LT::Str						    m_accrualCalendar;
			LT::Str						    m_fixingCalendar;
			bool						    m_isShortLeg;
			bool						    m_isReferenceLeg;
			LTQC::StubType					m_stubType;

            FloatLeg(const IDeA::DepositRateMktConvention& depRateMktConvention, const LTQC::Tenor& frequency, const LTQC::DayCountMethod& accrualBasis,
					const LT::Str& rollConvention, const LT::Str& accrualCalendar, const LT::Str& fixingCalendar, bool isReferenceLeg):
					m_depRateMktConvention(depRateMktConvention), m_frequency(frequency), m_accrualBasis(accrualBasis),
					m_rollConvention(rollConvention), m_accrualCalendar(accrualCalendar), m_fixingCalendar(fixingCalendar), m_isReferenceLeg(isReferenceLeg), m_isShortLeg(false), m_stubType(LTQC::StubType::SE) {}
		};
		struct FixedLeg {
			LTQC::Tenor					m_frequency;		
			LTQC::DayCountMethod		m_accrualBasis;
			LT::Str						m_rollConvention;
			LT::Str						m_accrualCalendar;
			LTQC::StubType              m_stubType;
			LTQC::RollRuleMethod	    m_rollRuleConvention;

			FixedLeg() : m_stubType(LTQC::StubType::SE),  m_rollRuleConvention(LTQC::RollRuleMethod::BusinessEOM) {}

            FixedLeg(const LTQC::Tenor& frequency, const LTQC::DayCountMethod& accrualBasis,
					const LT::Str& rollConvention, const LT::Str& accrualCalendar):
					m_frequency(frequency), m_accrualBasis(accrualBasis),
					m_rollConvention(rollConvention), m_accrualCalendar(accrualCalendar), m_stubType(LTQC::StubType::SE),  m_rollRuleConvention(LTQC::RollRuleMethod::BusinessEOM) {}
		};

        TenorBasisSwapDetails(const LTQC::Tenor& spotDays, const LTQC::Currency& currency, const LT::Str& index, 
							  const FloatLeg& leg1, 
							  const FloatLeg& leg2, 
							  const FixedLeg& spreadLeg);

        TenorBasisSwapDetails(const LTQC::Tenor& spotDays, const LTQC::Currency& currency, const LT::Str& index, 
							  const FloatLeg& leg1, 
							  const FloatLeg& leg2);

		virtual TenorBasisSwapDetails* clone () const;

		LTQC::Currency				m_currency;
		LT::Str						m_index;
        LTQC::Tenor					m_spotDays;

		FloatLeg&	getShortLeg() { if (m_leg1.m_isShortLeg) return m_leg1; else return m_leg2;}
		FloatLeg&	getLongLeg() { if (m_leg1.m_isShortLeg) return m_leg2; else return m_leg1;}
		const FloatLeg&	getShortLeg() const { if (m_leg1.m_isShortLeg) return m_leg1; else return m_leg2;}
		const FloatLeg&	getLongLeg() const { if (m_leg1.m_isShortLeg) return m_leg2; else return m_leg1;}

		FloatLeg&	getLeg1() {  return m_leg1;}
		FloatLeg&	getLeg2() { return m_leg2;}
		const FloatLeg&	getLeg1() const {return m_leg1;}
		const FloatLeg&	getLeg2() const {return m_leg2;}

		const FixedLeg&	getSpreadLeg() const { return m_spreadleg;}

		FloatLeg&	getBasisReferenceLeg() { if (m_leg1.m_isReferenceLeg) return m_leg1; else return m_leg2;}
		const FloatLeg&	getBasisReferenceLeg() const { if (m_leg1.m_isReferenceLeg) return m_leg1; else return m_leg2;}
		FloatLeg&	getOtherLeg() { if (m_leg1.m_isReferenceLeg) return m_leg2; else return m_leg1;}
		const FloatLeg&	getOtherLeg() const { if (m_leg1.m_isReferenceLeg) return m_leg2; else return m_leg1;}

		void setSpreadLeg(const FixedLeg& spreadLeg);

		void finalize();

	protected:

		virtual void identifyShortLeg();
        void setSpreadLegFromShortLeg();

		FloatLeg					m_leg1;
		FloatLeg					m_leg2;
		FixedLeg					m_spreadleg;
		bool						m_spreadLegIsShortLeg;			
    };

    inline TenorBasisSwapDetails* TenorBasisSwapDetails::clone( ) const
	{
		return new TenorBasisSwapDetails( *this );
    }

	/// TenorBasisSwap represents a Tenor Basis Swap
    /// instrument that can be used to calibrate a model.
    class TenorBasisSwap : public CalibrationInstrument
    {
    public:
        TenorBasisSwap(const string& description,
                       const LT::date fixingDate,
                       const LT::date startDate,
                       const LT::date endDate,
					   const LT::date lastDate,
                       const double spread,
					   const TenorBasisSwapDetails& tradeDetails,
					   const LT::date valueDate,
					   const EndDateCalculationType	backStubEndDateCalculationType);

		TenorBasisSwap(const string& description,
                       const LT::date fixingDate,
                       const LT::date startDate,
                       const LT::date endDate,
					   const LT::date lastDate,
                       const double spread,
					   const TenorBasisSwapDetails& tradeDetails,
                       const LT::date valueDate,
					   const EndDateCalculationType	backStubEndDateCalculationType,
                       GlobalComponentCache& globalComponentCache);

        static std::string getName()
        {
            return "Tenor Basis";
        }

		static CalibrationInstrumentPtr create(const LTQuant::GenericData& instrumentParametersTable, 
											   const LT::Ptr<LT::date>& buildDate,
											   const LTQuant::GenericData& curveParametersTable,
											   const LTQuant::GenericDataPtr&);

        static void createInstruments(CalibrationInstruments& instruments,
                                      LTQuant::GenericDataPtr instrumentTable,
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);
    
		static void createInstrumentsOldFormat(CalibrationInstruments& instruments,
                                      LTQuant::GenericDataPtr instrumentTable,
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);

        static void createInstrumentsNewFormat(CalibrationInstruments& instruments,
                                      LTQuant::GenericDataPtr instrumentTable,
                                      LTQuant::GenericDataPtr data,
                                      GlobalComponentCache& globalComponentCache,
                                      const LTQuant::PriceSupplierPtr);

		static void updateInstruments(CalibrationInstruments& instrumentList, 
                                      LTQuant::GenericDataPtr instrumentTable, 
                                      size_t* instrumentIndex);

		static bool hasNewMarketDataFormat(const LTQuant::GenericDataPtr& instrumentTable);
		static bool hasNewMarketDataFormat(const LT::TablePtr& instrumentTable);

		static EndDateCalculationType::Enum_t defaultBackStubEndDataCalculationType()
		{
			return EndDateCalculationType::FromAccrualEndDate;
		}

        inline double getShortTenor() const	{	return m_shortTenor;	}
        inline double getLongTenor() const	{	return m_longTenor;		}
        inline double getSpreadLegTenor() const	{	return m_spreadLegTenor;		}

		// Returns the reference basis Tenor, usually 3M
		inline std::string getReferenceTenor() const { return m_details.getBasisReferenceLeg().m_frequency.asTenorString().string(); }
		// Returns the (non-reference) Tenor
		inline std::string getOtherTenor() const {	return m_details.getOtherLeg().m_frequency.asTenorString().string();			}
		/**
		* Get the accrual basis of the spread leg
		* 
		* @return the accrual basis of the spread leg
		*/
		inline std::string getSpreadLegAccrualBasis() const {	return m_details.getSpreadLeg().m_accrualBasis.asString().data();			}
		inline std::string getSpreadLegTenorDesc() const { return m_details.getSpreadLeg().m_frequency.asTenorString().string(); }

		inline std::string getLeg2AccrualBasis() const {	return m_details.getLeg2().m_accrualBasis.asString().data();			}
		inline std::string getLeg2TenorDesc() const { return m_details.getLeg2().m_frequency.asTenorString().string(); }
		inline std::string getLeg1AccrualBasis() const {	return m_details.getLeg1().m_accrualBasis.asString().data();			}
		inline std::string getLeg1TenorDesc() const { return m_details.getLeg1().m_frequency.asTenorString().string(); }

        virtual string getType() const
        {
            return TenorBasisSwap::getName();
        }

        virtual const double getMarketPrice() const
        {
            return 0.0;
        }

        virtual void finishCalibration(const BaseModelPtr model);

        virtual void reloadInternalState(const FlexYCF::CalibrationInstrumentPtr & src);

		//	Returns whether the maturity of the instrument is less than
		//	the Tenor of its "other" leg
		bool hasSyntheticOtherLeg() const;

        virtual const double computeModelPrice(const BaseModelPtr) const;
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
							      IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows);
		virtual void fillRepFlows(IDeA::AssetDomainConstPtr assetDomain,
                                  const BaseModel& model,
							      const double multiplier, 
							      IDeA::RepFlowsData<IDeA::Index>& indexRepFlows);
		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const;
		virtual std::ostream& print(std::ostream& out) const;         // Useful for testing

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        TenorBasisSwap(TenorBasisSwap const& original, CloneLookup& lookup);

    private:
		FloatingLegPtr createFloatingLeg(const TenorBasisSwapDetails::FloatLeg& floatLeg,  
										 const LT::date valueDate,
										 GlobalComponentCache* const globalComponentCache = 0) const;
		FixedLegPtr createFixedLeg(const TenorBasisSwapDetails::FixedLeg& fixLeg,
								   const LT::date valueDate,
								   GlobalComponentCache* const globalComponentCache = 0) const;

		virtual void fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const;
		virtual void fillCashFlowPVsTable(const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable) const;
        
        TenorBasisSwap(TenorBasisSwap const&); // deliberately disabled as won't clone properly

        const double m_referenceTenor; // reference Tenor of basis swap (usually 3M)
        const double m_otherTenor; 

		TenorBasisSwapDetails	m_details;
		TenorBasisSwapDetails	m_flooredDetails; // this is to hack 1D to 1M

		const double m_shortTenor;  // spread leg
        const double m_longTenor;   // non-spread leg
		const double m_spreadLegTenor; 
		
		// Formula is: m_longFloatingLeg - m_shortFloatingLeg = m_spreadFixedLeg
        FloatingLegPtr  m_shortFloatingLeg; 
        FloatingLegPtr  m_longFloatingLeg;
        FixedLegPtr     m_spreadFixedLeg;

		EndDateCalculationType	m_backStubEndDateCalculationType;
    };  //  TenorBasisSwap
    
    DECLARE_SMART_PTRS( TenorBasisSwap )

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_TENORBASISSWAP_H_INCLUDED