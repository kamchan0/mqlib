/*****************************************************************************

    CalibrationInstrument

	Base class for all calibration instruments.
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENT_H_INCLUDED

#include "LTQuantInitial.h"

//	FlexYCF
#include "Gradient.h"
#include "IHasCashFlows.h"
#include "CachedDerivInstrument.h"
#include "IHasRepFlows.h"
#include "ICloneLookup.h"

// IDeA
#include "FundingRepFlow.h"
#include "IndexRepFlow.h"

//	LTQuantLib
#include "lt/const_string.h"
#include "lt/ptr.h"

#include "ModuleStaticData\InternalInterface\IRIndexProperties.h"

namespace LTQuant
{
	class Problem;
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace IDeA
{
    class AssetDomain;
	class DictionaryKey;
    struct IRCurveMktConvention;
	typedef LT::Ptr<IRCurveMktConvention> IRCurveMktConventionPtr; 
}

namespace LTQC
{
	class LTQC::Tenor;
}

namespace FlexYCF
{    
    FWD_DECLARE_SMART_PTRS( BaseModel )
	FWD_DECLARE_SMART_PTRS( CurveType )

    FWD_DECLARE_SMART_PTRS( CalibrationInstrument )

    // Forward declarations
    class CloneLookup;
	
    /// CalibrationInstrument is the base class that any instrument
    /// class that can be used to calibrate a model in the Flexible
    /// Yield-Curve Framework must derive from. 
	class CalibrationInstrument: public CachedDerivInstrument,
								 public IHasCashFlows,
                                 public IHasRepFlows<IDeA::Funding>,
                                 public IHasRepFlows<IDeA::Index>,
                                 public ICloneLookup
    {
    public:
		CalibrationInstrument(const double rate,
							  const LT::Str& name,
							  const LT::Str& description,
							  const LT::date fixingDate,
                              const LT::date startDate,
                              const LT::date endDate,
							  const LT::Str& identity = "");

        virtual ~CalibrationInstrument() = 0 {};


        /// there is an implementation in this class that is responsible for capturing
        /// the cached values
        virtual void finishCalibration(const BaseModelPtr model) = 0;
	    virtual void setValues(const BaseModelPtr model);
        /// after finishCalibration the instrument is an empty shell that cannot value itself
        /// given a full representation of the instrument as src refresh yourself with
        /// enough information so it is possible to value this instr again
        virtual void reloadInternalState(const CalibrationInstrumentPtr& src)=0;
        /* 
		* Returns the fixing date of the instrument
		* 
		* @return fixing date
		*/
        inline LT::date     getFixingDate()      const { return m_fixingDate;     }

		/// Returns the start date of the instrument
        inline LT::date     getStartDate()      const { return m_startDate;     }

        /// Returns the end date of the instrument
        inline LT::date     getEndDate()        const { return m_endDate;       }

        /// Returns the rate of the instrument
		//	-->	RiskInstrument
        //	inline double   getRate()           const { return m_rate;          }

        /// Sets the rate of the instrument
        //inline void     setRate(const double rate)      { m_rate = rate;}
        virtual double getLastRelevantTime() const {return 1.0; };
        
        // Make it call its specialized instrument_type in derived classes.
        // not sure to keep this
        virtual std::string getType() const = 0; 

		// typically return the identity name same as the instrument table
		virtual const LT::Str& getIdentity() const
		{
			return m_identity;
		}
        
        /// Returns the market price of the instrument
        virtual const double getMarketPrice() const = 0;

        /// Computes the theoretical price of the instrument according
        /// to the specified model
        virtual const double computeModelPrice(const BaseModelPtr) const = 0;

        /// Computes the PV of the instrument
        virtual const double computePV(const BaseModelPtr model) const;

        /// Computes the basis point value of the instrument
        virtual const double computeBPV(const BaseModelPtr model) const;
    
        /// Returns the residual, defined as the difference between the
        /// model price of the instrument (according to a model)
        /// and its market price
        inline double getResidual(const BaseModelPtr& baseModel) const
        {
            return computeModelPrice(baseModel) - getMarketPrice();
        }	
        
        // Maybe NOT pure virtual if it is something complicated or even impossible for certain instruments
        //  in which case an optimizer could always use a proxy of the gradient based on getResidual.
        virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd) = 0;

		virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType) = 0;
		
		virtual void accumulateGradientDependentModel(BaseModel const& depModel, BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
										GradientIterator gradientEnd) 
		{
			USE(depModel);
			USE(baseModel);
			USE(multiplier);
			USE(gradientBegin);
			USE(gradientEnd);
		}

		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
										GradientIterator gradientEnd,
										bool spread) = 0;
		
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
										GradientIterator gradientEnd,
										bool spread) = 0;
		

        /// Updates the instrument
        virtual void update() = 0;

		/// Returns the initial guess of the knot-point y value for this instrument
		///	NOTE: to be pure virtual
		virtual double getVariableInitialGuess(const double /* flowTime */,
											   const BaseModel* const /* model */) const { return -1.; }	//= 0;


		// Add the instrument rate as a variable to the specified problem
		//	virtual void addRateToProblem(LTQuant::Problem& problem); //  = 0; 

		// Returns the derivative of the residual relative to the rate of the instrument
		virtual double calculateRateDerivative(const BaseModelPtr& model) const = 0;

		//	Returns the gradient of the derivative of the PV residual relative to the rate of the instrument
		virtual void accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const = 0;
		virtual void accumulateRateDerivativeGradient(const BaseModel& model,
													  double multiplier,
													  GradientIterator gradientBegin,
													  GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const = 0;

		// would eventually replace addRateToProblem and calculateRateDerivative
		inline void setParRate(const BaseModelPtr& model)
		{
			CachedDerivInstrument::setRate(computeParRate(model));
		}

		// Returns the par rate of the instrument
		virtual double computeParRate(const BaseModelPtr& model) = 0;

		/// Prints the instrument
        virtual std::ostream& print(std::ostream& out) const;

		///	IHasCashFlowsInterface
		virtual LTQuant::GenericDataPtr getCashFlows();
		virtual LTQuant::GenericDataPtr computeCashFlowPVs(const BaseModel& model);
		//	IHasRepFlows<IDeA::Funding> interface
        virtual void fillRepFlows(IDeA::AssetDomainConstPtr ,
                                  const BaseModel& ,
								  const double ,
                                  IDeA::RepFlowsData<IDeA::Funding>& )
		{
			//	Do nothing by default
		}
		//	IHasRepFlows<IDeA::Index> interface
        virtual void fillRepFlows(IDeA::AssetDomainConstPtr ,
                                  const BaseModel& ,
								  const double ,
                                  IDeA::RepFlowsData<IDeA::Index>& )
		{
			//	Do nothing by default
		}

		virtual double getDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData) const = 0;

		//	Returns whether the instrument was placed on the model
		virtual inline bool wasPlaced() const
		{
			return m_wasPlaced;
		}

		inline void setAsPlaced(const bool wasPlaced)
		{
			m_wasPlaced = wasPlaced;
		}
		
		/*virtual LTQC::DepositRateType depositRateType() const
		{
			return LTQC::DepositRateType::IBOR;
		}*/

		// implemented only for futures
		virtual void setConvexity(double) {}
	protected:
        CalibrationInstrument();
		
		void setStartDate(const LT::date startDate);
		void setEndDate(const LT::date endDate);


		double doGetDifferenceWithNewRate(const LTQuant::GenericData& instrumentListData,
										  const IDeA::DictionaryKey& instrumentKey,
										  const IDeA::DictionaryKey& descriptionKey,
										  const IDeA::DictionaryKey& rateKey) const;

		//	Tries to get the new rate of the instrument in the specified data
		//	at the specified index and returns true if it new rate is set
		virtual bool compareAndTryGetNewRate(const LTQuant::GenericData& instrumentData,
											 const size_t index,
											 const IDeA::DictionaryKey& descriptionKey,
											 const IDeA::DictionaryKey& rateKey,
											 double& newRate) const;

        //have all the flows been destroyed for performance reasons so the instrument cannot be valued
        bool        m_flowsRemoved;
    private:
		// Hook to allow instruments to fill the cash-flows table
		//	Default implementation fills with the start date and end date
		virtual void fillCashFlowsTable(LTQuant::GenericData& cashFlowsTable) const;
		virtual void fillCashFlowPVsTable(const BaseModel& model,
										  LTQuant::GenericData& cashFlowPVsTable) const;
        
		LT::date	m_fixingDate;
        LT::date	m_startDate;
        LT::date	m_endDate;
		bool		m_wasPlaced;
		LT::Str		m_identity;
	};  //  CalibrationInstrument

    inline const double CalibrationInstrument::computeBPV(const BaseModelPtr model) const
    {
        return oneBasisPoint( ) * calculateRateDerivative( model );
    }

    inline const double CalibrationInstrument::computePV(const BaseModelPtr model) const
    {
        return getResidual( model );
    }

  

    void fillStartAndEndDates(const LTQuant::GenericDataPtr& instrumentTable,
                              const size_t index,
                              const std::string& description,
							  const string& fixingCalendar,
							  const string& accrualCalendar,
							  const LTQC::Tenor& spotDays,
                              const LT::date& valueDate,
                              LT::date& startDate,
                              LT::date& endDate);

    void fillEndDate(const LTQuant::GenericDataPtr& instrumentTable,
                              const size_t index,
                              const std::string& description,
							  const string& accrualCalendar,
                              const LT::date& startDate,
                              LT::date& endDate);
 
    LT::date startDateFromCell(const LT::Cell& startDateAsCell,
                               const LT::date& buildDate,
							   const LT::Str&  fixingCalendar,
							   const LT::Str&  accCalendar,
							   const LTQC::Tenor& spotDays);
    
    LT::date startDateFromDateOrTenor(const LTQuant::GenericData& instrumentParametersTable,
                                      const size_t index,
									  const LT::Ptr<LT::date>& buildDate,
									  const LT::Str& fixingCalendar,
									  const LT::Str& accCalendar,
									  const LTQC::Tenor& spotDays,
                                      LT::date& actualBuildDate,
                                      LT::date& fixingDate);

	void setMaturityAndDates(const LTQuant::GenericData& instrumentParametersTable,
									    const LT::Ptr<LT::date>& buildDate,
										const LT::Str& fixingCalendar,
										const LT::Str& accCalendar,
										const LTQC::Tenor& spotDays,
									    std::string& maturity,
									    LT::date& actualBuildDate,
									    LT::date& fixingDate,
									    LT::date& startDate,
									    LT::date& endDate);

    void setMaturityAndDates(const LTQuant::GenericData& instrumentParametersTable,
									    const LT::Ptr<LT::date>& buildDate,
										const LT::Str& fixingCalendar,
										const LT::Str& accCalendar,
										const LTQC::Tenor& spotDays,
                                        const LT::date& startDate,
									    std::string& maturity,
									    LT::date& actualBuildDate,
									    LT::date& fixingDate,
									    LT::date& endDate);

	//	Obsolete: use IDeA_KEY + aliases instead
	std::string getDescriptionOrEquivalent(const LTQuant::GenericDataPtr& instrumentTable,
										   const std::string& equivalentTag,
										   const size_t index);

	//	Obsolete: use IDeA_KEY + aliases instead
	double getRateOrSpread(const LTQuant::GenericDataPtr& instrumentTable,
						   const size_t index);

	namespace 
	{

		std::ostream& operator<<(std::ostream& out, const CalibrationInstrument& calibrationInstrument)
		{
			return calibrationInstrument.print(out);
		}
	      
		std::ostream& operator<<(std::ostream& out, const CalibrationInstrumentPtr calibrationInstrument)
		{
			return calibrationInstrument->print(out);
		}
	    
		std::ostream& operator<<(std::ostream& out, const CalibrationInstrumentConstPtr calibrationInstrument)
		{
			return calibrationInstrument->print(out);
		}
	}

    /*
	* Create default properties 
	* Read properties from curve parameter of, if not found, from defaults
	*/
	IDeA::IRCurveMktConventionPtr createIRCurveMktConventions(const LTQuant::GenericData& curveParametersTable);
	IDeA::IRCurveMktConventionPtr createIRCurveMktConventions(const LTQuant::GenericData& curveParametersTable, const ModuleStaticData::IRIndexPropertiesPtr& indexProperties);
	IDeA::IRCurveMktConventionPtr createIRCurveMktConventions(const LTQuant::GenericDataPtr& curveParametersTable);
	
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_CALIBRATIONINSTRUMENT_H_INCLUDED