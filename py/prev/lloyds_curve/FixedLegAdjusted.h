/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_FIXEDLEGADJUSTED_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_FIXEDLEGADJUSTED_H_INCLUDED
#pragma once

// FlexYCF
#include "FixedLeg.h"

namespace FlexYCF
{
    // Represents a BaseLeg (Fixed or Floating leg, df by dependent model)
	// adjusted by the AdjustedComponent (with main model) on a flow-by-flow basis
	template<class AdjComponentArgument>
	class FixedLegAdjusted :  public InstrumentComponent, public ICloneLookup
							, public IHasCashFlows
							, public IHasRepFlows<IDeA::Funding>
    {
    private: 
		typedef GenericInstrumentComponent<AdjComponentArgument> AdjustmentComponent;
		typedef std::vector<std::tr1::shared_ptr<AdjustmentComponent> > AdjustmentFlows;
		typedef typename AdjustmentFlows::const_iterator const_iterator_adjComponent;
		typedef typename AdjComponentArgument::Creator_Type AdjArgCreator;
		typedef FixedLeg::const_iterator const_iter_cfs;

    public:

		explicit FixedLegAdjusted(const FixedLegPtr& fixedLeg, const AdjArgCreator& adjArgCreator)
			: m_fixedLeg(fixedLeg), m_adjustmentArgCreator(adjArgCreator)
        {};
            
        static std::tr1::shared_ptr<FixedLegAdjusted<AdjustmentComponent>> 
			create(const FixedLegPtr& fixedLeg, const AdjArgCreator& adjArgCreator)
        {
            return std::tr1::shared_ptr<FixedLegAdjusted<AdjustmentComponent>>
					(new FixedLegAdjusted(fixedLeg,adjArgCreator));
        }

		virtual void initilize()
		{
			m_fixedLeg->initializeCashFlowPricing();
			doInitializeAdjustmentFlows();
			if(!m_dependentModel)
				LTQC_THROW( IDeA::ModelException, "Dependent model for the FixedLegAdjusted component is not yet set" );
		}

		void setDependentModel(const BaseModelConstPtr& model)
		{
			m_dependentModel = model;
		}

        virtual double getValue(BaseModel const& baseModel);

        virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd);

		virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);

		virtual void accumulateGradientConstantDiscountFactor(
										BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);
		
		virtual void accumulateGradientConstantTenorDiscountFactor(
										BaseModel const& baseModel, BaseModel const& dfModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										bool spread);

		virtual FixedLegPtr getFixedLeg() const { return m_fixedLeg; }
        
		virtual void update()
		{
			m_fixedLeg->update();
			std::for_each(m_adjustmentFlows.begin(),m_adjustmentFlows.end(), [](AdjustmentFlows::value_type& p){p->update();});
		}

        virtual std::ostream& print(std::ostream& out) const
		{
			out << "Fxd Leg Adjusted" << std::endl;
			// TO DO: output cash-flows
			return out;
		}

		/**
		@brief Clone this instance.

		Use a lookup to preserve directed graph relationships.

		@param lookup A lookup of previous created clones.
		@return       Returns a clone.
		*/
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const
		{
			return ICloneLookupPtr(new FixedLegAdjusted<AdjComponentArgument>(*this, lookup));
		}

        /// hold on to the schedules, but not the cash flows
		virtual void cleanupCashFlows()
		{
			m_fixedLeg->cleanupCashFlows();
			m_adjustmentFlows.clear();
		}

		// IHasCashFlows interface
		virtual LTQuant::GenericDataPtr computeCashFlowPVs(const BaseModel& model);
		virtual LTQuant::GenericDataPtr getCashFlows()
		{
			LTQuant::GenericDataPtr result = m_fixedLeg->getCashFlows();
			result->table->nameSet(getLegTypeName());
			return result;
		}
		
		// IHasRepFlows<Funding> interface
		virtual void fillRepFlows(	IDeA::AssetDomainConstPtr assetDomain,
									const BaseModel& model,
									const double multiplier,
									IDeA::RepFlowsData<IDeA::Funding>& fundingRepFlows)
		{
			m_fixedLeg->fillRepFlows(assetDomain, model, multiplier, fundingRepFlows);
		}

    protected:
        FixedLegAdjusted(FixedLegAdjusted const& original, CloneLookup& lookup);

    private:
        FixedLegAdjusted(FixedLegAdjusted const&); // deliberately disabled as won't clone properly

		virtual std::string getLegTypeName() const
		{
			return "Fixed Leg Adjusted";
		}

		virtual void doInitializeAdjustmentFlows();		
		
		FixedLegPtr			m_fixedLeg;
		AdjustmentFlows		m_adjustmentFlows;
		AdjArgCreator		m_adjustmentArgCreator;
		BaseModelConstPtr	m_dependentModel;

    };  //  FixedLegAdjusted

	template<class AdjComponentArgument>
	double FixedLegAdjusted<AdjComponentArgument>::getValue(BaseModel const& baseModel)
	{
		initilize();

		double tmpCashFlowSum = 0.0;

		const_iterator_adjComponent iterAdj(m_adjustmentFlows.begin());
		for(const_iter_cfs	iterFixed(m_fixedLeg->m_cashFlows.begin()); iterFixed != m_fixedLeg->m_cashFlows.end(); ++iterFixed, ++iterAdj)
			tmpCashFlowSum += iterFixed->first * iterFixed->second->getValue(*m_dependentModel) * (*iterAdj)->getValue(baseModel);  // cvg * df * adjustment

		return tmpCashFlowSum;
	}

	template<class AdjComponentArgument>
	void FixedLegAdjusted<AdjComponentArgument>::accumulateGradient(	
													BaseModel const& baseModel, 
													double multiplier, 
													GradientIterator gradientBegin, 
													GradientIterator gradientEnd)
	{
		initilize();

		const_iterator_adjComponent iterAdj(m_adjustmentFlows.begin());
		for(const_iter_cfs iterFixed(m_fixedLeg->m_cashFlows.begin()); iterFixed != m_fixedLeg->m_cashFlows.end(); ++iterFixed, ++iterAdj)
		{
			// compute the gradient for this adjustment factor:
			// multiply it by the corresponding (m_dependentModel)-discounted fixed cash flow:
			(*iterAdj)->accumulateGradient(baseModel, multiplier * iterFixed->first * iterFixed->second->getValue(*m_dependentModel), gradientBegin, gradientEnd);
		}
	}

	template<class AdjComponentArgument>
	void FixedLegAdjusted<AdjComponentArgument>::accumulateGradient(	
													BaseModel const& baseModel, 
													double multiplier, 
													GradientIterator gradientBegin, 
													GradientIterator gradientEnd, 
													const CurveTypeConstPtr& curveType)
	{
		initilize();

		const_iterator_adjComponent iterAdj(m_adjustmentFlows.begin());
		for(const_iter_cfs iterFixed(m_fixedLeg->m_cashFlows.begin()); iterFixed != m_fixedLeg->m_cashFlows.end(); ++iterFixed, ++iterAdj)
		{
			// compute the gradient for this adjustment factor:
			// multiply it by the corresponding (m_dependentModel)-discounted fixed cash flow:
			(*iterAdj)->accumulateGradient(baseModel, multiplier * iterFixed->first * iterFixed->second->getValue(*m_dependentModel), gradientBegin, gradientEnd, curveType);
		}
	}

	template<class AdjComponentArgument>
	void FixedLegAdjusted<AdjComponentArgument>::accumulateGradientConstantDiscountFactor(
													BaseModel const& baseModel, BaseModel const& dfModel, 
													double multiplier,
													GradientIterator gradientBegin, 
													GradientIterator gradientEnd,
													bool spread)
	{
		initilize();

		const_iterator_adjComponent iterAdj(m_adjustmentFlows.begin());
		for(const_iter_cfs iterFixed(m_fixedLeg->m_cashFlows.begin()); iterFixed != m_fixedLeg->m_cashFlows.end(); ++iterFixed, ++iterAdj)
			(*iterAdj)->accumulateGradientConstantDiscountFactor(	baseModel, dfModel, 
																multiplier * iterFixed->first * iterFixed->second->getValue(*m_dependentModel), 
																gradientBegin, gradientEnd, spread);
	}

	template<class AdjComponentArgument>
	void FixedLegAdjusted<AdjComponentArgument>::accumulateGradientConstantTenorDiscountFactor(
													BaseModel const& baseModel, BaseModel const& dfModel, 
													double multiplier,
													GradientIterator gradientBegin, 
													GradientIterator gradientEnd,
													bool spread)
	{
		initilize();

		const_iterator_adjComponent iterAdj(m_adjustmentFlows.begin());
		for(const_iter_cfs iterFixed(m_fixedLeg->m_cashFlows.begin()); iterFixed != m_fixedLeg->m_cashFlows.end(); ++iterFixed, ++iterAdj)
			(*iterAdj)->accumulateGradientConstantTenorDiscountFactor(	baseModel, dfModel,
																	multiplier * iterFixed->first * iterFixed->second->getValue(*m_dependentModel), 
																	gradientBegin, gradientEnd,spread);
	}

    /**
        @brief Pseudo copy constructor.

        Use a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
	template<class AdjComponentArgument>
    FixedLegAdjusted<AdjComponentArgument>::FixedLegAdjusted(FixedLegAdjusted const& original, CloneLookup& lookup)
    {
        // Copy across the adjusted factors
        m_adjustmentFlows.reserve(original.m_adjustmentFlows.size());
		for( AdjustmentFlows::const_iterator it = original.m_adjustmentFlows.begin(); it != original.m_adjustmentFlows.end(); ++it)
			m_adjustmentFlows.push_back(lookup.get(*it));
		m_fixedLeg = FixedLegPtr(new FixedLeg(*original.m_fixedLeg, lookup));
		
		m_dependentModel = original.m_dependentModel;
		m_adjustmentArgCreator = original.m_adjustmentArgCreator;		
    }

}   // FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_FIXEDLEGADJUSTED_H_INCLUDED