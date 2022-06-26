/*****************************************************************************
    
	AnalyticalDelta

	This file contains the implementation of functions to calculate
	analytical delta relative to risk instruments


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "AnalyticalDelta.h"
#include "BaseModel.h"
#include "LeastSquaresResiduals.h"
#include "InstrumentResidual.h"
#include "CalibrationInstrument.h"
#include "TenorBasisSwap.h"
#include "WeightedResidual.h"
#include "FlexYcfUtils.h"
#include "Gradient.h"
#include "ReplicatingFlows.h"
#include "IndexRepFlow.h"
#include "FundingRepFlow.h"
#include "InstrumentDelta.h"
#include "QLYieldCurve.h"
#include "MultiTenorModel.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"

//	LTQuantLib
#include "Data/GenericData.h"
#include "ModuleDate/InternalInterface/Utils.h"

//	LTQuantCore
#include "utils/QCException.h"

using namespace LTQC;
using namespace std;
using namespace IDeA;

namespace FlexYCF
{


	
	//	Calculates delta using funding and index replicating flows according the following formula:
	//
	//							D[V/R]  =  D[V/P]  x  J[P/z]  x  -( J[z/C]  x  J[C/R] )
	//
	// where:
	//	D[V/R] is a (row) vector of size N of the instrument whose replicating flows are passed to the function
	//	D[V/P] is the (row) vector of size M of all replicating flows
	//	J[P/z] is the MxN jacobian of the funding and index factors relative to the unknowns of the model
	//	J[z/C] = J[C/z]^{-1} is the NxN jacobian of the unknowns relative to the PV of the "partial" instruments
	//	J[C/R] is NxN (diagonal) jacobian of the sensitivy of the instrument PV relative to their market rate
    //  The '-' sign in the above formula is because C(z, R) = 0 defines an implicit function relating z and R.
    //  An application of the implicit function theorem to C gives this result.
	void calculateIRAnalyticalDelta(const BaseModel& model,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
								  InstrumentDeltaVector& deltaVector)
	{

        // integrity check
        if (!model.isJacobianSupported()) {
				LT_THROW_ERROR("Attempting to calculate algorithmic risk on a model that does not support Jacobian");
        }

		//	1.a Build a matrix whose rows are made of 
		//	the gradient of the funding rep flows
		//	and multiply on the left by the row vector
		//	containing the rep flow values. This
		//	amounts to doing a running weighted summation on the
		//	(funding) discount factor gradients at rep flow date,
		//	the weights being the rep flow values
		Gradient modRepFlows(model.getLeastSquaresResiduals()->size(), 0.0);
		
		for(IDeA::ReplicatingFlows<IDeA::Funding>::const_iterator iter(fundingRepFlows.begin()); iter != fundingRepFlows.end(); ++iter)
		{
			model.accumulateDiscountFactorGradient(iter->getTime(), iter->getValue(), modRepFlows.begin(), modRepFlows.end());
		}

		// 1.b Index Rep Flows
		//	Note: essentially the same as for funding rep flows, except
		//	for the fact that we use the tenor to call the index discount
		//	factor gradient accumulation function of the model
		for(IDeA::ReplicatingFlows<IDeA::Index>::const_iterator iter(indexRepFlows.begin()); iter != indexRepFlows.end(); ++iter)
		{
			model.accumulateTenorDiscountFactorGradient(iter->getTime(), iter->getTenor(), iter->getValue(), modRepFlows.begin(), modRepFlows.end());
		}

		//	2. Get the last jacobian of the model and invert it
		//	Important Note: we assume the jacobian is made of lines
		//	that represent the gradient of the *PV* of the instrument
		//	relative to the model unknowns
		LTQC::Matrix invJac(model.getJacobian());
		invJac.inverse();

		// Get all input instruments:
		CachedDerivInstruments fullInstruments(model.getFullPrecomputedInstruments());
		
		//	Clean delta vector if necessary:
		deltaVector.clear();
		deltaVector.resize(fullInstruments.size());
		
		//	3. Multiply on the right with a diagonal matrix whose
		//	elements are the derivative of the instrument PV relative
		//	to their market rate. This amounts to multiplying each column
		//	of the inverted jacobian by those values.
		//	Compute the multiplication of the line vector obtained in 1.
		//	by the modified inverted Jacobian from 2 - this is the risk
		const BaseModelPtr model_ptr(FlexYCF::toSharedPtr<BaseModel>(model));
		size_t j(0);					// running index on partial instruments
		double deltaValue(0.0), hedgeRatio(0.0);

		for(size_t k(0); k < fullInstruments.size(); ++k)
		{
			deltaValue = 0.0;
            hedgeRatio = 0.0;

			if(fullInstruments[k]->wasPlaced())
			{
				for(size_t i(0); i < invJac.getNumRows(); ++i)
				{
					deltaValue += (modRepFlows[i] * invJac(i,j));
				}

				// Multiply by rate derivative and scale to get delta value:
				deltaValue *= - oneBasisPoint() * fullInstruments[k]->getRateDerivative();
                const double bpv = fullInstruments[k]->getBPV();
                hedgeRatio = deltaValue / bpv;
				++j;
			}

			//	Put the delta value for this instrument into an InstrumentDelta instead of directly filling the table
			deltaVector[k] = InstrumentDelta(*fullInstruments[k], deltaValue, hedgeRatio, getDeltaType(*fullInstruments[k]));
		}

		//	Post-checking:
		if(invJac.getNumCols() != j)
		{
			LTQC_THROW( LTQC::ModelQCException, "The number of partial instruments is not equal to the number of columns of the inverse Jacobian of the solved problem" );
		}
	}
	
	void calculateIRAnalyticalDelta(IDeA::AssetDomainConstPtr modelAssetDomain,
									IDeA::AssetDomainConstPtr childModelAssetDomain,
								  const BaseModel& model,
								  const BaseModel& childModel,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
								  InstrumentDeltaVector& deltaVector,
								  double fx)
	{

        // integrity check
        if (!model.isJacobianSupported() || !childModel.isJacobianSupported()) {
				LT_THROW_ERROR("Attempting to calculate algorithmic risk on a model that does not support Jacobian");
        }

		LTQC::Matrix invJac(childModel.getFullJacobian());
		invJac.inverse();
		
		Gradient modRepFlows( invJac.getNumRows() - childModel.getLeastSquaresResiduals()->size(), 0.0);
		Gradient modRepFlowsChildModel(childModel.getLeastSquaresResiduals()->size(), 0.0);
		
		for(IDeA::ReplicatingFlows<IDeA::Funding>::const_iterator iter(fundingRepFlows.begin()); iter != fundingRepFlows.end(); ++iter)
		{
			childModel.accumulateBaseDiscountFactorGradient(iter->getTime(), iter->getValue(), modRepFlows.begin(), modRepFlows.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Index>::const_iterator iter(indexRepFlows.begin()); iter != indexRepFlows.end(); ++iter)
		{
			childModel.accumulateBaseTenorDiscountFactorGradient(iter->getTime(), iter->getTenor(), iter->getValue(), modRepFlows.begin(), modRepFlows.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Funding>::const_iterator iter(fundingRepFlows.begin()); iter != fundingRepFlows.end(); ++iter)
		{
			childModel.accumulateSpreadDiscountFactorGradient(iter->getTime(), iter->getValue(), modRepFlowsChildModel.begin(), modRepFlowsChildModel.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Index>::const_iterator iter(indexRepFlows.begin()); iter != indexRepFlows.end(); ++iter)
		{
			childModel.accumulateSpreadTenorDiscountFactorGradient(iter->getTime(), iter->getTenor(), iter->getValue(), modRepFlowsChildModel.begin(), modRepFlowsChildModel.end());
		}
		
		LTQC::Currency reportingCcy(modelAssetDomain->primaryDomain());
		LTQC::Currency childModelCcy(childModelAssetDomain->primaryDomain());

		// Get all input instruments:
		CachedDerivInstruments fullInstruments(model.getFullPrecomputedInstruments());
		

		
		deltaVector.clear();
		deltaVector.resize(fullInstruments.size());
		
		
		size_t j = childModel.jacobianOffset(modelAssetDomain);;					// running index on partial instruments
		double deltaValue(0.0), hedgeRatio(0.0);

		for(size_t k(0); k < fullInstruments.size(); ++k)
		{
			deltaValue = 0.0;
            hedgeRatio = 0.0;

			if(fullInstruments[k]->wasPlaced())
			{
				for(size_t i(0); i < invJac.getNumRows(); ++i)
				{
					if( i < modRepFlows.size() )
					{
						deltaValue += (modRepFlows[i] * invJac(i,j));
					}
					else
					{
						deltaValue += (modRepFlowsChildModel[i - modRepFlows.size()] * invJac(i,j));
					}
				}

				// Multiply by rate derivative and scale to get delta value:
				deltaValue = -  fx * deltaValue * oneBasisPoint() * fullInstruments[k]->getRateDerivative();
                const double bpv = fullInstruments[k]->getBPV();
                hedgeRatio = deltaValue / bpv;
				++j;
			}

			//	Put the delta value for this instrument into an InstrumentDelta instead of directly filling the table
			deltaVector[k] = InstrumentDelta(*fullInstruments[k], deltaValue, hedgeRatio, getDeltaType(*fullInstruments[k]));
		}
	}
	void calculateIRAnalyticalDelta(IDeA::AssetDomainConstPtr modelAssetDomain,
									IDeA::AssetDomainConstPtr childModelAssetDomain,
								  const BaseModel& model,
								  const BaseModel& childModel,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlowsChild,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlowsChild,
								  InstrumentDeltaVector& deltaVector,
								  double fx)
	{

        // integrity check
        if (!model.isJacobianSupported() || !childModel.isJacobianSupported()) {
				LT_THROW_ERROR("Attempting to calculate algorithmic risk on a model that does not support Jacobian");
        }

		LTQC::Matrix invJac(childModel.getFullJacobian());
		invJac.inverse();
		
		Gradient modRepFlows( invJac.getNumRows() - childModel.getLeastSquaresResiduals()->size(), 0.0);
		Gradient modRepFlowsChildModel(childModel.getLeastSquaresResiduals()->size(), 0.0);
		
		for(IDeA::ReplicatingFlows<IDeA::Funding>::const_iterator iter(fundingRepFlows.begin()); iter != fundingRepFlows.end(); ++iter)
		{
			model.accumulateDiscountFactorGradient(iter->getTime(), iter->getValue(), modRepFlows.begin(), modRepFlows.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Index>::const_iterator iter(indexRepFlows.begin()); iter != indexRepFlows.end(); ++iter)
		{
			model.accumulateTenorDiscountFactorGradient(iter->getTime(), iter->getTenor(), iter->getValue(), modRepFlows.begin(), modRepFlows.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Funding>::const_iterator iter(fundingRepFlowsChild.begin()); iter != fundingRepFlowsChild.end(); ++iter)
		{
			childModel.accumulateDiscountFactorGradient(iter->getTime(), iter->getValue(), modRepFlowsChildModel.begin(), modRepFlowsChildModel.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Index>::const_iterator iter(indexRepFlowsChild.begin()); iter != indexRepFlowsChild.end(); ++iter)
		{
			childModel.accumulateTenorDiscountFactorGradient(iter->getTime(), iter->getTenor(), iter->getValue(), modRepFlowsChildModel.begin(), modRepFlowsChildModel.end());
		}
		
		Currency reportingCcy(modelAssetDomain->primaryDomain());
		Currency childModelCcy(childModelAssetDomain->primaryDomain());

		// Get all input instruments:
		CachedDerivInstruments fullInstruments(model.getFullPrecomputedInstruments());
		

		
		deltaVector.clear();
		deltaVector.resize(fullInstruments.size());
		
		
		size_t j = childModel.jacobianOffset(modelAssetDomain);;					// running index on partial instruments
		double deltaValue(0.0), hedgeRatio(0.0);

		for(size_t k(0); k < fullInstruments.size(); ++k)
		{
			deltaValue = 0.0;
            hedgeRatio = 0.0;

			if(fullInstruments[k]->wasPlaced())
			{
				for(size_t i(0); i < invJac.getNumRows(); ++i)
				{
					if( i < modRepFlows.size() )
					{
						deltaValue += (modRepFlows[i] * invJac(i,j));
					}
					else
					{
						deltaValue +=  (modRepFlowsChildModel[i - modRepFlows.size()] * invJac(i,j));
					}
				}

				// Multiply by rate derivative and scale to get delta value:
				deltaValue = - fx * deltaValue * oneBasisPoint() * fullInstruments[k]->getRateDerivative();
                const double bpv = fullInstruments[k]->getBPV();
                hedgeRatio = deltaValue / bpv;
				++j;
			}

			//	Put the delta value for this instrument into an InstrumentDelta instead of directly filling the table
			deltaVector[k] = InstrumentDelta(*fullInstruments[k], deltaValue, hedgeRatio, getDeltaType(*fullInstruments[k]));
		}
	}
	void calculateIRAnalyticalDelta(IDeA::AssetDomainConstPtr modelAssetDomain,
									IDeA::AssetDomainConstPtr childModelAssetDomain,
									IDeA::AssetDomainConstPtr childChildModelAssetDomain,
								  const BaseModel& model,
								  const BaseModel& childModel,
								   const BaseModel& childChildModel,
								  const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
								  const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
								  InstrumentDeltaVector& deltaVector,
								  double fx)
	{

        // integrity check
        if (!model.isJacobianSupported() || !childModel.isJacobianSupported() || !childChildModel.isJacobianSupported()) {
				LT_THROW_ERROR("Attempting to calculate algorithmic risk on a model that does not support Jacobian");
        }

		LTQC::Matrix invJac(childChildModel.getFullJacobian());
		invJac.inverse();
		
		Gradient modRepFlowsChildModel(invJac.getNumRows()  - childChildModel.getLeastSquaresResiduals()->size(), 0.0);
		Gradient modRepFlowsChildChildModel(childChildModel.getLeastSquaresResiduals()->size(), 0.0);

	
		for(IDeA::ReplicatingFlows<IDeA::Funding>::const_iterator iter(fundingRepFlows.begin()); iter != fundingRepFlows.end(); ++iter)
		{
			childChildModel.accumulateBaseDiscountFactorGradient(iter->getTime(), iter->getValue(), modRepFlowsChildModel.begin(), modRepFlowsChildModel.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Index>::const_iterator iter(indexRepFlows.begin()); iter != indexRepFlows.end(); ++iter)
		{
			childChildModel.accumulateBaseTenorDiscountFactorGradient(iter->getTime(), iter->getTenor(), iter->getValue(), modRepFlowsChildModel.begin(), modRepFlowsChildModel.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Funding>::const_iterator iter(fundingRepFlows.begin()); iter != fundingRepFlows.end(); ++iter)
		{
			childChildModel.accumulateSpreadDiscountFactorGradient(iter->getTime(), iter->getValue(), modRepFlowsChildChildModel.begin(), modRepFlowsChildChildModel.end());
		}
		for(IDeA::ReplicatingFlows<IDeA::Index>::const_iterator iter(indexRepFlows.begin()); iter != indexRepFlows.end(); ++iter)
		{
			childChildModel.accumulateSpreadTenorDiscountFactorGradient(iter->getTime(), iter->getTenor(), iter->getValue(), modRepFlowsChildChildModel.begin(), modRepFlowsChildChildModel.end());
		}
		Currency reportingCcy(modelAssetDomain->primaryDomain());
		Currency childModelCcy(childModelAssetDomain->primaryDomain());
		Currency childChildModelCcy(childChildModelAssetDomain->primaryDomain());
		// Get all input instruments:
		CachedDerivInstruments fullInstruments(model.getFullPrecomputedInstruments());
		

		
		deltaVector.clear();
		deltaVector.resize(fullInstruments.size());
		
		
		size_t j = childChildModel.jacobianOffset(modelAssetDomain);					// running index on partial instruments
		double deltaValue(0.0), hedgeRatio(0.0);

		for(size_t k(0); k < fullInstruments.size(); ++k)
		{
			deltaValue = 0.0;
            hedgeRatio = 0.0;

			if(fullInstruments[k]->wasPlaced())
			{
				for(size_t i(0); i < invJac.getNumRows(); ++i)
				{
					if( i < modRepFlowsChildModel.size() )
					{
						deltaValue += (modRepFlowsChildModel[i] * invJac(i,j));
					} 
					else if( i >= modRepFlowsChildModel.size() )
					{
						deltaValue += (modRepFlowsChildChildModel[i - modRepFlowsChildModel.size()] * invJac(i,j));
					}
				}

				// Multiply by rate derivative and scale to get delta value:
				deltaValue = - fx * deltaValue * oneBasisPoint() * fullInstruments[k]->getRateDerivative();
                const double bpv = fullInstruments[k]->getBPV();
                hedgeRatio = deltaValue / bpv;
				++j;
			}

			//	Put the delta value for this instrument into an InstrumentDelta instead of directly filling the table
			deltaVector[k] = InstrumentDelta(*fullInstruments[k], deltaValue, hedgeRatio, getDeltaType(*fullInstruments[k]));
		}
	}
    //	Calculates inflation delta using inflation replicating flows according the following formula (explained above):
	//
	//							D[V/R]  =  D[V/P]  x  J[P/z]  x  -( J[z/C]  x  J[C/R] )
     void calculateILAnalyticalDelta(const BaseModel& model,
								     const IDeA::ReplicatingFlows<IDeA::Inflation>& inflationRepFlows,
								     InstrumentDeltaVector& deltaVector)
     {
        //	1. Build a matrix whose rows are made of 
		//	the gradient of the inflation rep flows
		//	and multiply on the left by the row vector
		//	containing the rep flow values. This
		//	amounts to doing a running weighted summation on the
		//	(inflation) index forward gradients at rep flow date,
		//	the weights being the rep flow values
		Gradient modRepFlows(model.getLeastSquaresResiduals()->size(), 0.0);
		
		for(IDeA::ReplicatingFlows<IDeA::Inflation>::const_iterator iter(inflationRepFlows.begin()); iter != inflationRepFlows.end(); ++iter)
		{
			model.accumulateDiscountFactorGradient(iter->getTime(), iter->getValue(), modRepFlows.begin(), modRepFlows.end());
		}

        //	2. Get the last jacobian of the model and invert it
		//	Important Note: we assume the jacobian is made of lines
		//	that represent the gradient of the *PV* of the instrument
		//	relative to the model unknowns
		LTQC::Matrix invJac(model.getJacobian());
		invJac.inverse();

        // Get all input instruments:
		CachedDerivInstruments fullInstruments(model.getFullPrecomputedInstruments());
		
		//	Clean delta vector if necessary:
		deltaVector.clear();
		deltaVector.resize(fullInstruments.size());

        //	3. Multiply on the right with a diagonal matrix whose
		//	elements are the derivative of the instrument PV relative
		//	to their market rate. This amounts to multiplying each column
		//	of the inverted jacobian by those values.
		//	Compute the multiplication of the line vector obtained in 1.
		//	by the modified inverted Jacobian from 2 - this is the risk
        const BaseModelPtr model_ptr(FlexYCF::toSharedPtr<BaseModel>(model));
        size_t j(0);					// running index on partial instruments
		double deltaValue(0.0), hedgeRatio(0.0);

		for(size_t k(0); k < fullInstruments.size(); ++k)
		{
			deltaValue = 0.0;
            hedgeRatio = 0.0;

			if(fullInstruments[k]->wasPlaced())
			{
				for(size_t i(0); i < invJac.getNumRows(); ++i)
				{
					deltaValue += (modRepFlows[i] * invJac(i,j));
				}

				// Multiply by rate derivative and scale to get delta value:
				deltaValue *= - oneBasisPoint() * fullInstruments[k]->getRateDerivative();
                const double bpv = fullInstruments[k]->getBPV();
                hedgeRatio = deltaValue / bpv;
				++j;
			}

			//	Put the delta value for this instrument into an InstrumentDelta instead of directly filling the table
			deltaVector[k] = InstrumentDelta(*fullInstruments[k], deltaValue, hedgeRatio, getDeltaType(*fullInstruments[k]));
		}

		//	Post-checking:
		if(invJac.getNumCols() != j)
		{
			LTQC_THROW( LTQC::ModelQCException, "The number of partial instruments is not equal to the number of columns of the inverse Jacobian of the solved problem" );
		}
     }

	
	//	Calculates delta relative to structure rates according to the following formula:
	//
	//								D[V/R]  =  D[V/P]  x  J[P/R]	
	//
	//	where:
	//	R represents the structure instrument rates
	//	J[P/R] is the jacobian of the funding and index factors relative to the structure rates
	//	see the definitions above for D[V/R] and D[V/P]
	//
	//	Note:
	//	because df' = - df logFvf', for any funding or index discount at time t and log Fvf value at time t,
	//	where the derivative is understood to be relative to any structure rate, the calculation is
	//	essentially the sum of the line vectors in J[P/R] weighted by a factor of - "repFlow" x df x 1 bp
	void calculateStructureAnalyticalDelta(const BaseModel& model,
										   const IDeA::ReplicatingFlows<IDeA::Funding>& fundingRepFlows,
										   const IDeA::ReplicatingFlows<IDeA::Index>& indexRepFlows,
										   InstrumentDeltaVector& deltaVector)
	{
		const InstrumentDeltaVector structureDeltaVector(model.getStructure().calculateDelta(model, fundingRepFlows,indexRepFlows));
		//	Copy for now... might want to add (i.e. append) structure delta at the end of specified delta vector
		deltaVector = structureDeltaVector;
	}
	
	FlexYCF::InstrumentDeltaVector calculateRisk(FlexYCF::BaseModelPtr model, AssetDomainConstPtr assetDomain, vector<YieldCurveIFConstPtr> constChildYC, vector<LT::Str> childModelAD, RepFlowsData<Funding> fundingRepFlows, RepFlowsData<IDeA::Index> indexRepFlows, const std::map<LT::Str, double>& fxRates)
	{
		ReplicatingFlows<Funding> fRepFlows( fundingRepFlows.toRepFlows( model->getValueDate( ), assetDomain ) );
        ReplicatingFlows<IDeA::Index> iRepFlows( indexRepFlows.toRepFlows( model->getValueDate( ), assetDomain ) );
		FlexYCF::InstrumentDeltaVector fullDeltaVector;
		if( constChildYC.empty() )
		{
			FlexYCF::calculateIRAnalyticalDelta( *model, fRepFlows, iRepFlows, fullDeltaVector );
		}
		else
		{
			vector< ReplicatingFlows<IDeA::Index> > indicesRepFlows;
			std::set<AssetDomainConstPtr,AssetDomainConstPtrLessThan> ads = indexRepFlows.assetDomains();
			std::set<AssetDomainConstPtr,AssetDomainConstPtrLessThan>::const_iterator it = ads.begin();
			for( ;it != ads.end(); ++it)
			{
				ReplicatingFlows<IDeA::Index> iFlows( indexRepFlows.toRepFlows( model->getValueDate( ), *it ) );
				indicesRepFlows.push_back(iFlows);
			}
		
			vector< ReplicatingFlows<Funding> > fundingsRepFlows;
			std::set<AssetDomainConstPtr,AssetDomainConstPtrLessThan> adFunding = fundingRepFlows.assetDomains();
			std::set<AssetDomainConstPtr,AssetDomainConstPtrLessThan>::const_iterator itF = adFunding.begin();
			for( ;itF != adFunding.end(); ++itF)
			{
				ReplicatingFlows<Funding> fFlows( fundingRepFlows.toRepFlows( model->getValueDate( ), *itF ) );
				fundingsRepFlows.push_back(fFlows);
			}
		
			// Multitenor
			AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[0]);
			YieldCurveIF* ycChildModel = const_cast<YieldCurveIF*>(constChildYC[0].get());
			IDeA::QLYieldCurve* qlycChildModel = dynamic_cast<IDeA::QLYieldCurve*>(ycChildModel);
			FlexYCF::BaseModelPtr childModel = qlycChildModel->getModel();
			if(std::tr1::dynamic_pointer_cast<MultiTenorModel>(childModel) && constChildYC.size() == 1)
			{
				FlexYCF::InstrumentDeltaVector fullDeltaVector;
				ReplicatingFlows<IDeA::Funding> fundingRepFlows, fundingRepFlowsChild;
				ReplicatingFlows<IDeA::Index> indexRepFlows, indexRepFlowsChild;

				for( size_t k = 0; k < fundingsRepFlows.size(); ++k)
				{
					if(fundingsRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(assetDomain->discriminator()) == 0 )
					{
						fundingRepFlows = fundingsRepFlows[k];
					}
					AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[0]);
					if(fundingsRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(childModelAssetDomain->discriminator()) == 0 )
					{
						fundingRepFlowsChild = fundingsRepFlows[k];
					}
				}
				for( size_t k = 0; k < indicesRepFlows.size(); ++k)
				{
					if(indicesRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(assetDomain->discriminator()) == 0 )
					{
						indexRepFlows = indicesRepFlows[k];
					}
					AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[0]);
					if(indicesRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(childModelAssetDomain->discriminator()) == 0 )
					{
						indexRepFlowsChild = indicesRepFlows[k];
					}
				}

				FlexYCF::InstrumentDeltaVector fundingDeltaVector;
				AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[0]);
				YieldCurveIF* ycChildModel = const_cast<YieldCurveIF*>(constChildYC[0].get());
				IDeA::QLYieldCurve* qlycChildModel = dynamic_cast<IDeA::QLYieldCurve*>(ycChildModel);
				FlexYCF::BaseModelPtr childModel = qlycChildModel->getModel();
				auto it = fxRates.find(childModelAssetDomain->primaryDomain());	
				ReplicatingFlows<IDeA::Index> emptyIndexRepFlows;
				ReplicatingFlows<IDeA::Funding> emptyFundingRepFlows;
				FlexYCF::calculateIRAnalyticalDelta( assetDomain,childModelAssetDomain, *model, *childModel, emptyFundingRepFlows, emptyIndexRepFlows, fundingRepFlowsChild, indexRepFlowsChild, fundingDeltaVector, it->second );
				FlexYCF::mergeInstrumentDeltas(fullDeltaVector, fundingDeltaVector);
				
				fundingDeltaVector.clear();
				FlexYCF::calculateIRAnalyticalDelta( assetDomain,childModelAssetDomain, *model, *childModel, fundingRepFlows, indexRepFlows, emptyFundingRepFlows, emptyIndexRepFlows, fundingDeltaVector, 1.0 );
				FlexYCF::mergeInstrumentDeltas(fullDeltaVector, fundingDeltaVector);
				
				return fullDeltaVector;
			}
			if(std::tr1::dynamic_pointer_cast<MultiTenorModel>(childModel) && constChildYC.size() == 2 && childModel->hasDependentModel(*assetDomain))
			{
				FlexYCF::InstrumentDeltaVector fullDeltaVector;
				ReplicatingFlows<IDeA::Funding> fundingRepFlows, fundingRepFlowsChild, fundingRepFlowsChildChild;
				ReplicatingFlows<IDeA::Index> indexRepFlows, indexRepFlowsChild, indexRepFlowsChildChild;

				for( size_t k = 0; k < fundingsRepFlows.size(); ++k)
				{
					if(fundingsRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(assetDomain->discriminator()) == 0 )
					{
						fundingRepFlows = fundingsRepFlows[k];
					}
					AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[0]);
					if(fundingsRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(childModelAssetDomain->discriminator()) == 0 )
					{
						fundingRepFlowsChild = fundingsRepFlows[k];
					}
					AssetDomainConstPtr childChildModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[1]);
					if(fundingsRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(childChildModelAssetDomain->discriminator()) == 0 )
					{
						fundingRepFlowsChildChild = fundingsRepFlows[k];
					}
				}
				for( size_t k = 0; k < indicesRepFlows.size(); ++k)
				{
					if(indicesRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(assetDomain->discriminator()) == 0 )
					{
						indexRepFlows = indicesRepFlows[k];
					}
					AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[0]);
					if(indicesRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(childModelAssetDomain->discriminator()) == 0 )
					{
						indexRepFlowsChild = indicesRepFlows[k];
					}
					AssetDomainConstPtr childChildModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[1]);
					if(indicesRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(childChildModelAssetDomain->discriminator()) == 0 )
					{
						indexRepFlowsChildChild = indicesRepFlows[k];
					}
				}

				FlexYCF::InstrumentDeltaVector fundingDeltaVector;
				AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[0]);
				AssetDomainConstPtr childChildModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[1]);
				YieldCurveIF* ycChildModel = const_cast<YieldCurveIF*>(constChildYC[0].get());
				IDeA::QLYieldCurve* qlycChildModel = dynamic_cast<IDeA::QLYieldCurve*>(ycChildModel);
				FlexYCF::BaseModelPtr childModel = qlycChildModel->getModel();
				YieldCurveIF* ycChildChildModel = const_cast<YieldCurveIF*>(constChildYC[1].get());
				IDeA::QLYieldCurve* qlycChildChildModel = dynamic_cast<IDeA::QLYieldCurve*>(ycChildChildModel);
				FlexYCF::BaseModelPtr childChildModel = qlycChildChildModel->getModel();

				auto it = fxRates.find(childModelAssetDomain->primaryDomain());	
				ReplicatingFlows<IDeA::Index> emptyIndexRepFlows;
				ReplicatingFlows<IDeA::Funding> emptyFundingRepFlows;

				FlexYCF::calculateIRAnalyticalDelta( assetDomain,childModelAssetDomain, childChildModelAssetDomain,*model, *childModel, *childChildModel, fundingRepFlowsChildChild, indexRepFlowsChildChild, fundingDeltaVector, it->second);
				FlexYCF::mergeInstrumentDeltas(fullDeltaVector, fundingDeltaVector);
				
				
				fundingDeltaVector.clear();
				FlexYCF::calculateIRAnalyticalDelta( assetDomain,childModelAssetDomain, *model, *childModel, emptyFundingRepFlows, emptyIndexRepFlows, fundingRepFlowsChild, indexRepFlowsChild, fundingDeltaVector, it->second );
				FlexYCF::mergeInstrumentDeltas(fullDeltaVector, fundingDeltaVector);
				
				fundingDeltaVector.clear();
				FlexYCF::calculateIRAnalyticalDelta( assetDomain,childModelAssetDomain, *model, *childModel, fundingRepFlows, indexRepFlows, emptyFundingRepFlows, emptyIndexRepFlows, fundingDeltaVector, 1.0 );
				FlexYCF::mergeInstrumentDeltas(fullDeltaVector, fundingDeltaVector);

				return fullDeltaVector;
			}
			for( size_t k = 0; k < fundingsRepFlows.size(); ++k)
			{
				FlexYCF::InstrumentDeltaVector fundingDeltaVector;
				if(fundingsRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(assetDomain->discriminator()) == 0 )
				{
					ReplicatingFlows<IDeA::Index> emptyRepFlows;	
					FlexYCF::calculateIRAnalyticalDelta( *model, fundingsRepFlows[k], emptyRepFlows, fundingDeltaVector);
					FlexYCF::mergeInstrumentDeltas(fullDeltaVector, fundingDeltaVector);
				}
				else
				{
					for(size_t i = 0; i < constChildYC.size(); ++i)
					{	
						AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[i]);
						if(fundingsRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(childModelAssetDomain->discriminator()) == 0)
						{
							YieldCurveIF* ycChildModel = const_cast<YieldCurveIF*>(constChildYC[i].get());
							IDeA::QLYieldCurve* qlycChildModel = dynamic_cast<IDeA::QLYieldCurve*>(ycChildModel);
							FlexYCF::BaseModelPtr childModel = qlycChildModel->getModel();
							ReplicatingFlows<IDeA::Index> emptyRepFlows;
							auto it = fxRates.find(childModelAssetDomain->primaryDomain());
							if( it != fxRates.end() )
							{
								FlexYCF::calculateIRAnalyticalDelta( assetDomain,childModelAssetDomain, *model, *childModel, fundingsRepFlows[k], emptyRepFlows, fundingDeltaVector,it->second);
								FlexYCF::mergeInstrumentDeltas(fullDeltaVector, fundingDeltaVector);
							}
							else
							{
								LTQC_THROW( ModelException, "Can not find fx rate for " <<  childModelAssetDomain->primaryDomain().data() << " and " << assetDomain->primaryDomain().data() );
							}
						}
					}
				}
			}

			for( size_t k = 0; k < indicesRepFlows.size(); ++k)
			{
				FlexYCF::InstrumentDeltaVector indexDeltaVector;
				if(indicesRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(assetDomain->discriminator()) == 0 )
				{
					ReplicatingFlows<Funding> emptyRepFlows;	
					FlexYCF::calculateIRAnalyticalDelta( *model, emptyRepFlows, indicesRepFlows[k], indexDeltaVector );
					FlexYCF::mergeInstrumentDeltas(fullDeltaVector, indexDeltaVector);
				}
				else
				{
					for(size_t i = 0; i < constChildYC.size(); ++i)
					{	
						AssetDomainConstPtr childModelAssetDomain = AssetDomain::createAssetDomain(childModelAD[i]);
						if(indicesRepFlows[k].begin()->getAssetDomain()->discriminator().compareCaseless(childModelAssetDomain->discriminator()) == 0)
						{
							YieldCurveIF* ycChildModel = const_cast<YieldCurveIF*>(constChildYC[i].get());
							IDeA::QLYieldCurve* qlycChildModel = dynamic_cast<IDeA::QLYieldCurve*>(ycChildModel);
							FlexYCF::BaseModelPtr childModel = qlycChildModel->getModel();
							ReplicatingFlows<Funding> emptyFundFlows;
							auto it = fxRates.find(childModelAssetDomain->primaryDomain());
							if( it != fxRates.end() )
							{
								FlexYCF::calculateIRAnalyticalDelta( assetDomain,childModelAssetDomain, *model, *childModel, emptyFundFlows, indicesRepFlows[k], indexDeltaVector,it->second);
								FlexYCF::mergeInstrumentDeltas(fullDeltaVector, indexDeltaVector);
							}
							else
							{
								LTQC_THROW( ModelException, "Can not find fx rate for " <<  childModelAssetDomain->primaryDomain().data() << " and " << assetDomain->primaryDomain().data() );
							}
						}
					}
				}
			}
		}
		return fullDeltaVector;
	}
	
}