/*****************************************************************************

	BaseModel

	BaseModel is the base class for all models in FlexYCF.

    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BASEMODEL_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BASEMODEL_H_INCLUDED
#pragma once

#include "LTQuantInitial.h"
#include "FlexYCFZeroCurve.h"
#include "AssetDomain.h"
#include "CalibrationInstruments.h"
#include "CachedDerivInstruments.h"
#include "KnotPoint.h"
#include "ModuleDate/InternalInterface/Schedule.h"
#include "Gradient.h"
#include "CurveType.h"
#include "StructureHolder.h"
#include "ICloneLookup.h"

//	LTQuantCore
#include "Matrix.h"

namespace LTQuant
{
	class GDTblWrapper; 
    typedef GDTblWrapper GenericData; 

    FWD_DECLARE_SMART_PTRS( Problem )
}

namespace QDDate
{
    FWD_DECLARE_SMART_PTRS( DayCounter )  
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BaseKnotPointPlacement )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )
    FWD_DECLARE_SMART_PTRS( CurveFormulation )
    FWD_DECLARE_SMART_PTRS( BaseModel )
	FWD_DECLARE_SMART_PTRS( SpineDataCache )

	class IKnotPointFunctor;

    /// An abstract class for implementations
    /// of yield curve models
	class BaseModel: protected StructureSurfaceHolder, public ICloneLookup
    {
    public:

		static void restoreModelSpineData(SpineDataCachePtr& sdp);

        virtual ~BaseModel() {};

        /// Returns the value date
        LT::date getValueDate() const;
       
        /// Returns the value of a discount factor at the 
        /// specified time 
        virtual double getDiscountFactor(const double flowTime) const = 0;
        
        /// Returns the value of the Tenor discount factor 
        /// at the specified time
        virtual double getTenorDiscountFactor(const double flowTime, 
                                               const double tenor) const = 0;

		virtual double getTenorDiscountFactor(double flowTime, double tenor, const LTQC::Currency& ccy, const LT::Str& index) const;
		
		virtual double getSpreadDiscountFactor(const double flowTime) const 
		{
			USE(flowTime)
			return 1.0;
		}

        virtual double getSpreadTenorDiscountFactor(const double flowTime, const double tenor) const
		{
			USE(flowTime)
			USE(tenor)
			return 1.0; 
		}
		
		virtual double getBaseDiscountFactor(const double flowTime) const 
		{
			return getDiscountFactor(flowTime);
		}

        /**
        * Return the structure factor that is exogenously computed
        * @param flowTime the time in year fraction
        * @return the structure factor
        */
        virtual double getStructureFactor(const double flowTime) const
        {
            return getStructure().getDiscountFactor(flowTime);
        }

        virtual double getBaseTenorDiscountFactor(const double flowTime, const double tenor) const
		{
			return getTenorDiscountFactor(flowTime, tenor); 
		}

		/// Returns the discount factor of the spine curve 
		//	of the specified curve type
		virtual double getSpineDiscountFactor(const double flowTime,
											  const CurveTypeConstPtr& curveType) const = 0;

        /// Computes the gradient of the discount factor at 
        /// the specified time relative to the unknowns
        virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd) const = 0;
		
		virtual void accumulateSpreadDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const 
		{
			USE(flowTime)
			USE(multiplier)
			USE(gradientBegin)
			USE(gradientEnd)
		}
		
		virtual void accumulateBaseDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
													  GradientIterator gradientEnd) const 
		{
			 accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
		}
        
		/// Computes the gradient of the Tenor discount factor 
        ///  at the specified time relative to the unknowns
        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor,
                                                           double multiplier,
                                                           GradientIterator gradientBegin,
                                                           GradientIterator gradientEnd) const = 0;

		virtual void accumulateSpreadTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor,
                                                           double multiplier,
                                                           GradientIterator gradientBegin,
                                                           GradientIterator gradientEnd) const
		{
			USE(flowTime)
			USE(tenor)
			USE(multiplier)
			USE(gradientBegin)
			USE(gradientEnd)	
		}
		
		virtual void accumulateBaseTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor,
                                                           double multiplier,
                                                           GradientIterator gradientBegin,
                                                           GradientIterator gradientEnd) const
		{
			accumulateTenorDiscountFactorGradient(flowTime, tenor, multiplier, gradientBegin, gradientEnd);
		}
		
		// For multi-curve models, accumulates the gradient of the discount factor 
		//	for variables relative to the specified curve type 
		// For single-curve models, accumulates the gradient of the discount factor
		// Note: the distance between gradientBegin and gradientEnd must be equal to the number
		// of variables on curve of the specified curve 
		virtual void accumulateDiscountFactorGradient(const double flowTime, 
                                                      double multiplier,
                                                      GradientIterator gradientBegin,
                                                      GradientIterator gradientEnd,
													  const CurveTypeConstPtr& curveType) const = 0;

		// For multi-curve models, accumulates the gradient of the Tenor discount factor
		//	for variables relative to the specified curve type
		// For single-curve models, accumulates the gradient of the Tenor discount factor
		// Note: the distance between gradientBegin and gradientEnd must be equal to the number
		// of variables on curve of the specified curve
		//  *!*  The gradient is computed for the nearest LTQC::Tenor curve on the Tenor Spread       *!*
        //  *!*     Surface for now and not necessarily computed at the exact Tenor specified   *!*
        //  *!*     (this would imply stipulating an interpolation method along tenors)         *!*
        virtual void accumulateTenorDiscountFactorGradient(const double flowTime,
                                                           const double tenor, 
                                                           double multiplier, 
														   GradientIterator gradientBegin, 
                                                           GradientIterator gradientEnd,
														   const CurveTypeConstPtr& curveType) const = 0;
        
		
		/* Those will be needed for structure risk:
		virtual void accumulateFundingDiscountFactorStructureGradient(const double flowTime,
																	  double multiplier,
																	  GradientIterator gradientBegin,
																	  GradientIterator gradientEnd) const = 0;
		virtual void accumulateIndexDiscountFactorStructureGradient(const double flowTime,
																	const double tenor,
																    double multiplier,
																	GradientIterator gradientBegin,
																	GradientIterator gradientEnd) const = 0;		
		*/

		/// Returns the residuals that appear
        /// in the least squares sum
        LeastSquaresResidualsPtr getLeastSquaresResiduals() const;

        /// Sets the knot-point placement
        void setKnotPointPlacement(const BaseKnotPointPlacementPtr kppPtr);
    
        /// Returns the knot-point placement
        BaseKnotPointPlacementPtr getKnotPointPlacement() const;

        /// Places the knots and returns whether the placement 
        /// has succeeded
        bool placeKnotPoints(CalibrationInstruments& instruments);

        /// Initializes the knot-points
        virtual void initializeKnotPoints() { }// = 0;

		virtual void onInitialized() 
		{
			// do nothing by default
		}

		/// Returns the value of a variable from the time and
		///	the discount factor of the spine curve of the specified curve type
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
															   const double discountFactor,
															   const CurveTypeConstPtr& curveType) const = 0;
		
		/// Returns the details of the spine curves
		virtual LTQuant::GenericDataPtr getSpineCurvesDetails() const;

        /// Adds the unfixed spine curves knot points to the list
        virtual void getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const = 0;

		virtual void getSpineInternalData(SpineDataCachePtr& sdp) const = 0;

		virtual void assignSpineInternalData(SpineDataCachePtr& sdp) = 0;

        /// Updates the state of the model
        /// Usage note: should be called once the variables of
        /// the model have changed
        virtual void update() = 0;

        /// Finalizes the model
        virtual void finalize() = 0;

        /// Adds the variables of the model to the problem
        virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem) = 0;
		virtual void addVariablesToProblem(const LTQuant::ProblemPtr& problem,
									   	   IKnotPointFunctor& onKnotPointVariableAddedToProblem) = 0;

		///	Update the variables from the shifts
		virtual void updateVariablesFromShifts(const LTQC::VectorDouble& variableShifts) = 0;

        // This version of the getJacobian methods can potentially include headings
        virtual LT::TablePtr getJacobian( const bool includeHeadings ) const;

		//	Returns the jacobian of the calibrated model
		inline const LTQC::Matrix& getJacobian() const
		{
			return m_jacobian;
		}

        inline const LTQC::Matrix& getInverseJacobian() const
		{
            if( m_inverseJacobian.empty() )
            {
                m_inverseJacobian = m_jacobian;
                m_inverseJacobian.inverse();
            }
            return m_inverseJacobian;
		}
		 
		inline const LTQC::Matrix& getInverseFullJacobian() const
		{
            if( m_inverseFullJacobian.empty() )
            {
                m_inverseFullJacobian = getFullJacobian();
                m_inverseFullJacobian.inverse();
            }
            return m_inverseFullJacobian;
		}
        
		//	Set the jacobian of the calibrated model
		inline void setJacobian(const LTQC::Matrix& jacobian)
		{
			m_jacobian = jacobian;
            m_inverseJacobian.clear();
			m_inverseFullJacobian.clear();
		}

		//	Returns the structure curve
		inline const StructureSurface& getStructure() const
		{
			return StructureSurfaceHolder::holdee();
		}

        const LTQuant::GenericDataPtr   getDependentMarketData() const 
        {
            return m_dependentMarketData;
        }

        /// after calib is finished destory all of the calibration
        /// instruments so they do not take up memory via the CachedInstrument component
        virtual void finishCalibration()=0;
        /// Prints the model
        virtual std::ostream& print(std::ostream& out) const;


        //	Returns all the input instruments passed to the model
        //  N.B. These instruments may no longer be fully constructed

        //  external clients of the model should only be using getFullPrecomputedInstruments

        inline const CachedDerivInstruments getFullPrecomputedInstruments() const
        {
            return CachedDerivInstruments(m_fullInstruments);
        }

        /*** Possible design extension: add a knot-point to the CurveFormulation        ***/
        /*** with the specified description, not yet implemented for concrete models    ***/
        // void addKnotPoint(const string& description, const KnotPoint& knotPoint) const;
        
        void JacobianIsNotSupported() const
        {
            m_isJacobianSupported = false;
        }

        bool isJacobianSupported() const
        {
            return  m_isJacobianSupported;
        }

        bool hasDependentMarketData() const
        {
            bool hdm = ((m_dependentMarketData && (m_dependentMarketData->numItems() > 1)) ? true : false);
            return hdm;
        }
        
        bool hasDependentIRMarketData(const LT::Str& currency, const LT::Str& index) const;
        bool hasDependentFXSpotMarketData(const LT::Str& fgnCcy, const LT::Str& domCcy) const;

        static BaseModelPtr getZeroCurveModel(const LTQuant::ZeroCurvePtr zc);
        LTQuant::GenericDataPtr getDependencies() const 
        {
            return m_dependencies;
        };

        //TODO
        virtual void prepareForSolve();

        // returns the dependent model given the ccy and index
        BaseModelConstPtr getDependentModel(const IDeA::AssetDomain& id) const;
        bool hasDependentModel(const IDeA::AssetDomain& id) const;
        bool hasDependentModel(const string& ccy, const string& index) const;
        // returns the dependent fxrate
        double getDependentFXRate(const IDeA::AssetDomain& id) const;
        
		virtual  const LTQC::Matrix& getFullJacobian() const
		{
			if( m_dependentModels.empty() )
			{
				return m_jacobian;
			}
			else
			{
				return getFullJacobian();
			}
		}
		
		virtual LT::TablePtr getFullJacobian( const bool includeHeadings ) const
        {
			if( m_dependentModels.empty() )
				return getJacobian( includeHeadings );
			else
				return getFullJacobian(includeHeadings);
        }

		

		size_t numberOfPlacedInstruments() const
		{
			size_t k = 0;
			CalibrationInstruments instruments = getFullInstruments();
			for(size_t i = 0; i < instruments.size(); ++i)
			{
				if( instruments[i]->wasPlaced() )
				{
					++k;
				}
			}
			return k;
		}
		
		virtual size_t jacobianOffset(IDeA::AssetDomainConstPtr ad) const
		{
			return 0;
		}

		virtual IDeA::AssetDomainConstPtr  primaryAssetDomainFunding() const { return m_primaryAssetDomainFunding; }
		virtual IDeA::AssetDomainConstPtr  primaryAssetDomainIndex()   const { return m_primaryAssetDomainIndex; }
        void  setPrimaryAssetDomainFunding(IDeA::AssetDomainConstPtr ad) const { m_primaryAssetDomainFunding = ad; }
		void  setPrimaryAssetDomainIndex(IDeA::AssetDomainConstPtr ad)   const { m_primaryAssetDomainIndex = ad; }

		virtual void setCalibrated() {}
		
    protected:
		BaseModel();
		explicit BaseModel(const LT::date& valueDate);
		explicit BaseModel(const LTQuant::GenericData& masterTable, const LTQuant::FlexYCFZeroCurvePtr parent);
        explicit BaseModel(BaseModel const& original, CloneLookup& lookup);

		void initializeLeastSquaresResiduals();

		void setValueDate(const LT::date valueDate);

        BaseModelPtr resolveYieldCurveModel(IDeA::AssetDomainConstPtr ad);

        double resolveFXSpotRates(IDeA::AssetDomainConstPtr ad);

        //	Returns all the input instruments passed to the model
        //  N.B. These instruments may no longer be fully constructed
        //  external clients of the model should only be using getFullPrecomputedInstruments
        virtual inline const CalibrationInstruments& getFullInstruments() const
        {
            return m_fullInstruments;
        }
		
		
    private:
        /// A hook to do additional things once the knot-points have been placed
        /// Useful to count the number of unknowns, add knot-points to a curve whole 
        /// whose knot-points formulation depends in a complex on the the formulation 
        /// of the initializing knot-points
        virtual void onKnotPointsPlaced() { }
        BaseModel(BaseModel const&); // deliberately disabled as won't clone properly

        LT::date					m_valueDate;
        BaseKnotPointPlacementPtr	m_knotPointPlacement;
        LeastSquaresResidualsPtr	m_leastSquaresResiduals;
		LTQC::Matrix				m_jacobian;
        
        // computed only if requested
        mutable LTQC::Matrix		m_inverseJacobian;
		mutable LTQC::Matrix		m_inverseFullJacobian;

        //	Note: m_fullInstruments contains ALL instruments,
		//	even those NOT used in calibration
		CalibrationInstruments		m_fullInstruments;

        LTQuant::FlexYCFZeroCurvePtr   m_parent;
        LTQuant::GenericDataPtr   m_dependentMarketData;
        LTQuant::GenericDataPtr   m_dependencies;
        mutable bool     m_isJacobianSupported;

        typedef std::map<IDeA::AssetDomainConstPtr, BaseModelConstPtr,  IDeA::AssetDomainConstPtrLessThan> StrBMMap;
        StrBMMap		m_dependentModels;
        
        typedef std::map<IDeA::AssetDomainConstPtr, double,  IDeA::AssetDomainConstPtrLessThan> StrFXRateMap;
        StrFXRateMap    m_dependentFXRates;

		mutable IDeA::AssetDomainConstPtr  m_primaryAssetDomainFunding;
        mutable IDeA::AssetDomainConstPtr  m_primaryAssetDomainIndex;
    };  //  Base Model
    
    DECLARE_SMART_PTRS( BaseModel )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const BaseModel& baseModel)
		{
			return baseModel.print(out);
		}
	}
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_BASEMODEL_H_INCLUDED