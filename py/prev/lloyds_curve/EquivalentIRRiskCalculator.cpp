#include "stdafx.h"

//	FlexYCF
#include "EquivalentIRRiskCalculator.h"
#include "EquivalentCurveFunctionsFactory.h"
#include "CalibrationInstruments.h"
#include "GlobalComponentCache.h"
#include "CalibrationInstrumentFactory.h"
#include "CalibrationInstruments.h"
#include "BaseModelFactory.h"
#include "BaseModel.h"
#include "Futures.h"
#include "TenorBasisSwap.h"
#include "StructureInstrumentUtils.h"
#include "FlexYCFZeroCurve.h"
#include "FundingRepFlow.h"
#include "IndexRepFlow.h"
#include "InstrumentDelta.h"
#include "AnalyticalDelta.h"

//	LTQuantLib
#include "Maths/LeastSquaresProblem.h"
#include "Maths/LeastSquaresSolverFactory.h"
#include "Maths/LeastSquaresSolver.h"
#include "Maths/LevenbergMarquardtSolver.h"
#include "Data/GenericData.h"
#include "Pricers/ZeroCurve.h"
#include "LTQuantUtils.h"
#include "GlobalComponentCache.h"
#include "CalibrationInstrumentFactory.h"
// Standard
#include <vector>
#include <string>

// IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"
#include "YieldCurveIF.h"
#include "QLYieldCurve.h"

// #include "UtilsEnums.h"
#include "YieldCurve.h"
#include "FunctorEnvelope.h"
#include "FactoryEnvelope.h"
#include "DictionaryItem.h"
#include "IDeASystem.h"
#include "FactoryEnvelopeH.h"
#include "RiskReportFactory.h"
#include "VectorDouble.h"
#include "TblConversion.h"

using namespace LTQC;
using namespace std;
using namespace IDeA;

namespace FlexYCF
{
	void riskFromWeights(BaseModelPtr model, const VectorDouble& inputWeights, FlexYCF::InstrumentDeltaVector& targetDeltaVector)
	{
		CachedDerivInstruments partialInstruments(model->getFullPrecomputedInstruments());
	
		vector<double> outputRisk(partialInstruments.size(), 0.0);
		targetDeltaVector.resize(partialInstruments.size());
        for(size_t i = 0, k = 0; i < partialInstruments.size(); ++i)
		{
			if( partialInstruments[i]->wasPlaced() )
			{
				outputRisk[i] = inputWeights[k] * partialInstruments[i]->getBPV();
				++k;
			}	
		}
		targetDeltaVector.resize(outputRisk.size());
		for(size_t k = 0; k < outputRisk.size(); ++k)
		{
			targetDeltaVector[k] = InstrumentDelta(*partialInstruments[k], outputRisk[k], 0.0, getDeltaType(*partialInstruments[k]));
		}

	}
	
	void placedInstrumentsWeights(BaseModelPtr model, const std::vector<double>& inputRisk, std::vector<double>& outputRisk)
	{
		CachedDerivInstruments partialInstruments(model->getFullPrecomputedInstruments());
		if(inputRisk.size() != partialInstruments.size())
		{
			LTQC_THROW(SystemException, "Size of source risk and the number of source yield curve don't match: risk " << inputRisk.size() << ", source curve " << partialInstruments.size());
		}
		
		for(size_t j = 0; j < partialInstruments.size(); ++j)
		{
			if(partialInstruments[j]->wasPlaced() )
			{
				outputRisk.push_back(inputRisk[j]/partialInstruments[j]->getBPV());
			}
			if(!partialInstruments[j]->wasPlaced() && fabs(inputRisk[j]) > 1.0E-03)
			{
				LTQC_THROW(SystemException, "Non zero risk for instrument not used in the curve " << partialInstruments[j]->getName().cStr() << ": " <<  partialInstruments[j]->getDescription().cStr());
			}
		}
	}

	void weightsFromRisk(BaseModelPtr model, VectorDouble& weights)
	{
		CachedDerivInstruments partialInstruments(model->getFullPrecomputedInstruments());
		for(size_t j = 0, k = 0; j < partialInstruments.size(); ++j)
		{
			if(partialInstruments[j]->wasPlaced() )
			{
				weights[k] /= partialInstruments[j]->getBPV();
				++k;
			}
		}
	}

	void calculateEquivalentIRRisk(LT::TablePtr sourceMasterTable, LT::TablePtr targetMasterTable, const IDeA::YieldCurveIFConstPtr& sourceYieldCurve,
								  const IDeA::YieldCurveIFConstPtr&  targetYieldCurve,
								  const vector<double>& sourceDeltaRisk,
								  const vector<double>& sourceSpreadDeltaRisk,
								  const std::vector<RiskReportFactory::IRRisk>& fullRisk,
								  IDeA::AssetDomainConstPtr assetDomain,
								  LT::TablePtr& irdeltaTbl)
    {
		const BaseModelPtr sourceModel(sourceYieldCurve->getModel());
		if(sourceModel == 0)
		{
			LT_THROW_ERROR( "The model in the FlexYCF source curve has not been built" );
		}
		const BaseModelPtr targetModel(targetYieldCurve->getModel());
		if(targetModel == 0)
		{
			LT_THROW_ERROR( "The model in the FlexYCF target curve has not been built" );
		}
		std::vector<IDeA::AssetDomainConstPtr> depSourceAssetDomains;
		std::vector<LT::TablePtr> depSourceMasterTables;
		getDependentModels(sourceMasterTable, depSourceAssetDomains, depSourceMasterTables);
		std::vector<IDeA::AssetDomainConstPtr> depTargetAssetDomains;
		std::vector<LT::TablePtr> depTargetMasterTables;
		getDependentModels(targetMasterTable, depTargetAssetDomains, depTargetMasterTables);

		LTQC::Matrix sourceToTarget, sourceToDepTarget, depSourceToDepTarget, depSource2ToDepTarget;



		////////////////////////////////
		std::vector<IDeA::YieldCurveIFConstPtr> depSourceModel, depTargetModels;
		std::vector<BaseModelPtr> depTarget;
		for(size_t i = 0; i < depSourceMasterTables.size(); ++i)
		{
			IDeA::YieldCurveIFConstPtr dep = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depSourceMasterTables[i])->getYieldCurve();
			depSourceModel.push_back(dep);
		}
		for(size_t i = 0; i < depTargetMasterTables.size(); ++i)
		{
			IDeA::YieldCurveIFConstPtr dep = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depTargetMasterTables[i])->getYieldCurve();
			depTargetModels.push_back(dep);
			depTarget.push_back(dep->getModel());
		}
		transitionMatrixSourceToTarget(targetYieldCurve,assetDomain,sourceMasterTable,sourceModel,targetModel,sourceToTarget);
		

		if(depSourceMasterTables.size() != depTargetMasterTables.size())
		{
			LT_THROW_ERROR( "The source and target models have different number of dependencies" );
		}

		if(depSourceMasterTables.size() > 0 && depTargetMasterTables.size() > 0)
		{
			IDeA::YieldCurveIFConstPtr depTarget = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depTargetMasterTables[0])->getYieldCurve();
			const BaseModelPtr depTargetModel(depTarget->getModel());
	
			transitionMatrixSourceToDepTarget(targetYieldCurve,assetDomain,sourceMasterTable, sourceModel,targetModel, depTargetModel, depTargetAssetDomains[0],sourceToDepTarget);
			//transitionMatrixSourceToDepTarget2(targetYieldCurve, depTargetModels[0], assetDomain,sourceMasterTable, sourceModel,targetModel, depTarget, depTargetAssetDomains,sourceToDepTarget);

	
			IDeA::YieldCurveIFConstPtr depSource = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depSourceMasterTables[0])->getYieldCurve();
			const BaseModelPtr depSourceModel(depSource->getModel());
			transitionMatrixDepToDep(depSourceMasterTables[0], depSourceModel,depTargetModel, depTargetModel, depTargetAssetDomains[0], depSourceToDepTarget);
			// transitionMatrixDepToDep2(assetDomain, targetYieldCurve, depTargetModels[0], depSourceMasterTables[0], depSourceModel,depTarget[0], depTarget, depTargetAssetDomains, depSourceToDepTarget);

			if(depSourceMasterTables.size() == 2)
			{
				IDeA::YieldCurveIFConstPtr depSource1 = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depSourceMasterTables[0])->getYieldCurve();
				const BaseModelPtr depSourceModel1(depSource1->getModel());
				IDeA::YieldCurveIFConstPtr depSource2 = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depSourceMasterTables[1])->getYieldCurve();
				const BaseModelPtr depSourceModel2(depSource2->getModel());
				//transitionMatrixDepToDep2(assetDomain, targetYieldCurve, depTargetModels[0], depSourceMasterTables[1], depSourceModel2,depTarget[0], depTarget, depTargetAssetDomains, depSource2ToDepTarget);
					
				FlexYCF::InstrumentDeltaVector deltaVector1, deltaVector2, spreadRiskVector;
				std::vector<double> sourceDepDeltaRisk = fullRisk[0].myRisk, sourceDep2DeltaRisk=fullRisk[1].myRisk;
						

				//collateDeltaDepRisk(sourceToDepTarget, depSourceToDepTarget,  depSource2ToDepTarget, sourceDepDeltaRisk, sourceDep2DeltaRisk, sourceSpreadDeltaRisk, sourceModel, depSourceModel1,  depSourceModel2, depTarget[0],  depTarget[1], deltaVector1, deltaVector2);
				collateRisk(sourceToTarget, sourceSpreadDeltaRisk, sourceModel, targetModel, spreadRiskVector);
				LT::TablePtr irdeltaRisk1Tbl = FlexYCF::toTable( deltaVector1 );
				LT::TablePtr irdeltaRisk2Tbl = FlexYCF::toTable( deltaVector2 );
				LT::TablePtr irspreadRiskTbl = FlexYCF::toTable( spreadRiskVector);
			
				std::vector<IDeA::AssetDomainConstPtr> allAD;
				allAD.push_back(depTargetAssetDomains[0]);
				allAD.push_back(depTargetAssetDomains[1]);
				allAD.push_back(assetDomain);

				std::vector<LT::TablePtr> deltaTbls;
				deltaTbls.push_back(irdeltaRisk1Tbl);
				deltaTbls.push_back(irdeltaRisk2Tbl);
				deltaTbls.push_back(irspreadRiskTbl);
				irdeltaTbl = RiskReportFactory::riskReport(allAD, deltaTbls);
				return;
			}
			

			FlexYCF::InstrumentDeltaVector deltaRiskVector, spreadRiskVector;
			collateDeltaRisk(sourceToDepTarget, depSourceToDepTarget, sourceDeltaRisk,sourceSpreadDeltaRisk, sourceModel, depSourceModel, depTargetModel, deltaRiskVector);
			collateRisk(sourceToTarget, sourceSpreadDeltaRisk, sourceModel, targetModel, spreadRiskVector);
			LT::TablePtr irdeltaRiskTbl = FlexYCF::toTable( deltaRiskVector );
			LT::TablePtr irspreadRiskTbl = FlexYCF::toTable( spreadRiskVector);
			
			std::vector<IDeA::AssetDomainConstPtr> allAD;
			allAD.push_back(depTargetAssetDomains[0]);
			allAD.push_back(assetDomain);

			std::vector<LT::TablePtr> deltaTbls;
			deltaTbls.push_back(irdeltaRiskTbl);
			deltaTbls.push_back(irspreadRiskTbl);
			irdeltaTbl = RiskReportFactory::riskReport(allAD, deltaTbls);
			return;
		}
	
		FlexYCF::InstrumentDeltaVector sourceToTargetFullDeltaVector;
		collateRisk(sourceToTarget, sourceDeltaRisk, sourceModel, targetModel, sourceToTargetFullDeltaVector);
		LT::TablePtr irdeltaRiskTbl = FlexYCF::toTable( sourceToTargetFullDeltaVector );

		std::vector<IDeA::AssetDomainConstPtr> allAD;
		allAD.push_back(assetDomain);

		std::vector<LT::TablePtr> deltaTbls;
		deltaTbls.push_back(irdeltaRiskTbl);
			
		irdeltaTbl = RiskReportFactory::riskReport(allAD, deltaTbls);
    }
	
	void calculateTransitionMatrix(LT::TablePtr sourceMasterTable, LT::TablePtr targetMasterTable, const IDeA::YieldCurveIFConstPtr& sourceYieldCurve,
								  const IDeA::YieldCurveIFConstPtr&  targetYieldCurve,
								  IDeA::AssetDomainConstPtr assetDomain,
								  LT::TablePtr& tm)
    {
		const BaseModelPtr sourceModel(sourceYieldCurve->getModel());
		if(sourceModel == 0)
		{
			LT_THROW_ERROR( "The model in the FlexYCF source curve has not been built" );
		}
		const BaseModelPtr targetModel(targetYieldCurve->getModel());
		if(targetModel == 0)
		{
			LT_THROW_ERROR( "The model in the FlexYCF target curve has not been built" );
		}
		std::vector<IDeA::AssetDomainConstPtr> depSourceAssetDomains;
		std::vector<LT::TablePtr> depSourceMasterTables;
		getDependentModels(sourceMasterTable, depSourceAssetDomains, depSourceMasterTables);
		std::vector<IDeA::AssetDomainConstPtr> depTargetAssetDomains;
		std::vector<LT::TablePtr> depTargetMasterTables;
		getDependentModels(targetMasterTable, depTargetAssetDomains, depTargetMasterTables);

		LTQC::Matrix sourceToTarget, sourceToDepTarget, depSourceToDepTarget, depSource2ToDepTarget;



		////////////////////////////////
		std::vector<IDeA::YieldCurveIFConstPtr> depSourceModel, depTargetModels;
		std::vector<BaseModelPtr> depTarget;
		for(size_t i = 0; i < depSourceMasterTables.size(); ++i)
		{
			IDeA::YieldCurveIFConstPtr dep = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depSourceMasterTables[i])->getYieldCurve();
			depSourceModel.push_back(dep);
		}
		for(size_t i = 0; i < depTargetMasterTables.size(); ++i)
		{
			IDeA::YieldCurveIFConstPtr dep = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depTargetMasterTables[i])->getYieldCurve();
			depTargetModels.push_back(dep);
			depTarget.push_back(dep->getModel());
		}

		transitionMatrixSourceToTarget(targetYieldCurve,assetDomain,sourceMasterTable,sourceModel,targetModel,sourceToTarget);
		
		if(depSourceMasterTables.size() != depTargetMasterTables.size())
		{
			LT_THROW_ERROR( "The source and target models have different number of dependencies" );
		}

		if(depSourceMasterTables.size() > 0 && depTargetMasterTables.size() > 0)
		{
			IDeA::YieldCurveIFConstPtr depTarget = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depTargetMasterTables[0])->getYieldCurve();
			const BaseModelPtr depTargetModel(depTarget->getModel());
	
			transitionMatrixSourceToDepTarget(targetYieldCurve,assetDomain,sourceMasterTable, sourceModel,targetModel, depTargetModel, depTargetAssetDomains[0],sourceToDepTarget);
			//transitionMatrixSourceToDepTarget2(targetYieldCurve, depTargetModels[0], assetDomain,sourceMasterTable, sourceModel,targetModel, depTarget, depTargetAssetDomains,sourceToDepTarget);

	
			IDeA::YieldCurveIFConstPtr depSource = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depSourceMasterTables[0])->getYieldCurve();
			const BaseModelPtr depSourceModel(depSource->getModel());
			transitionMatrixDepToDep(depSourceMasterTables[0], depSourceModel,depTargetModel, depTargetModel, depTargetAssetDomains[0], depSourceToDepTarget);
			// transitionMatrixDepToDep2(assetDomain, targetYieldCurve, depTargetModels[0], depSourceMasterTables[0], depSourceModel,depTarget[0], depTarget, depTargetAssetDomains, depSourceToDepTarget);
			
	
			if(depSourceMasterTables.size() == 2)
			{
				/*IDeA::YieldCurveIFConstPtr depSource1 = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depSourceMasterTables[0])->getYieldCurve();
				const BaseModelPtr depSourceModel1(depSource1->getModel());
				IDeA::YieldCurveIFConstPtr depSource2 = IDeA::FactoryEnvelopeH::getFunctorImpl<IDeA::YieldCurve>(depSourceMasterTables[1])->getYieldCurve();
				const BaseModelPtr depSourceModel2(depSource2->getModel());
				transitionMatrixDepToDep2(assetDomain, targetYieldCurve, depTargetModels[0], depSourceMasterTables[1], depSourceModel2,depTarget[0], depTarget, depTargetAssetDomains, depSource2ToDepTarget);
				size_t a = sourceToTarget.getNumRows();
				size_t b = depSource2ToDepTarget.getNumRows();
				size_t c = depSourceModel1->numberOfPlacedInstruments();
				size_t noRow = a + b;
				sourceToTarget.resize(noRow,noRow);
				for(size_t i = 0; i < noRow; ++i)
					for(size_t j = 0; j < noRow; ++j)
					{
						if( j > a && i < a )
							sourceToTarget(i,j) = 0.0;
						if(i >= a)
						{
							if(j < a)
								sourceToTarget(i,j) = sourceToDepTarget(i-a,j);
							if(j >= a && j < a+c)
								sourceToTarget(i,j) = depSourceToDepTarget(i-a,j-a);
							if(j >= a+c)
								sourceToTarget(i,j) = depSource2ToDepTarget(i-a,j-a-c);
						}

					}

				tm = LTQC::TblConversion_toTbl( sourceToTarget );		*/
				return;
			}

			size_t a = sourceToTarget.getNumRows();
			size_t b = sourceToDepTarget.getNumRows();
				
			size_t noRow = a + b;
			sourceToTarget.resize(noRow,noRow);
			for(size_t i = 0; i < noRow; ++i)
				for(size_t j = 0; j < noRow; ++j)
				{
					if( j > a && i < a )
						sourceToTarget(i,j) = 0.0;
					if(i >= a)
					{
						if(j < a)
							sourceToTarget(i,j) = sourceToDepTarget(i-a,j);
						if(j >= a && j < a+b)
							sourceToTarget(i,j) = depSourceToDepTarget(i-a,j-a);
					}

				}

			tm = LTQC::TblConversion_toTbl( sourceToTarget );	
			return;
		}
		tm = LTQC::TblConversion_toTbl( sourceToTarget );	
		return;
    }
	void dependentModel(LT::TablePtr masterTable, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr& depMasterTable)
	{
		LT::TablePtr dependencies;
		IDeA::permissive_extract<LT::TablePtr>(masterTable, IDeA_KEY(YIELDCURVE, DEPENDENCIES), dependencies);
		if (dependencies) {
			for (size_t i=1; i<dependencies->rowsGet();i++) {
				LT::Str key;
				bool isKeyGiven = IDeA::permissive_extract<LT::Str>(dependencies, IDeA_KEY(DEPENDENCIES, KEY), i-1, key);
				
				if (isKeyGiven) {
					AssetDomainConstPtr adKey(AssetDomain::createAssetDomain(key));
					if (assetDomain->discriminator().compareCaseless(adKey->discriminator()) == 0)
					{
						depMasterTable = IDeA::extract<LT::TablePtr>(dependencies, IDeA_KEY(DEPENDENCIES, OBJECT), i-1);
						return;
					}
				}
			}
		}
	}

	void getDependentModels(LT::TablePtr masterTable, std::vector<IDeA::AssetDomainConstPtr>& assetDomains, std::vector<LT::TablePtr>& depMasterTables)
	{
		LT::TablePtr dependencies;
		IDeA::permissive_extract<LT::TablePtr>(masterTable, IDeA_KEY(YIELDCURVE, DEPENDENCIES), dependencies);
		if (dependencies) {
			for (size_t i=1; i<dependencies->rowsGet();i++) {
				LT::Str key;
				bool isKeyGiven = IDeA::permissive_extract<LT::Str>(dependencies, IDeA_KEY(DEPENDENCIES, KEY), i-1, key);
				
				if (isKeyGiven) {
					AssetDomainConstPtr adKey(AssetDomain::createAssetDomain(key));
					if (adKey->getDomainType() == IDeA::AssetDomainType::IR)
					{
						LT::TablePtr table = IDeA::extract<LT::TablePtr>(dependencies, IDeA_KEY(DEPENDENCIES, OBJECT), i-1);
						depMasterTables.push_back(table);
						assetDomains.push_back(adKey);
					}
				}
			}
		}
	}


	void collateRisk(const LTQC::Matrix &A, const std::vector<double>& sourceDeltaRisk, BaseModelPtr sourceModel, BaseModelPtr targetModel, FlexYCF::InstrumentDeltaVector& targetFullDeltaVector)
	{
		CachedDerivInstruments sourcePartialInstruments(sourceModel->getFullPrecomputedInstruments());
		if(sourceDeltaRisk.size() != sourcePartialInstruments.size())
		{
			LTQC_THROW(SystemException, "Size of source risk and the number of source yield curve don't match: risk " << sourceDeltaRisk.size() << ", source curve " << sourcePartialInstruments.size());
		}
		vector<double> sourceRiskPlacedInstruments;
		placedInstrumentsWeights(sourceModel, sourceDeltaRisk, sourceRiskPlacedInstruments);
		

		VectorDouble result, sourceRisk(sourceRiskPlacedInstruments.begin(),sourceRiskPlacedInstruments.end());
		//weightsFromRisk(sourceModel, sourceRisk);
        dot_and_assign(A, sourceRisk, false, result);
		riskFromWeights(targetModel, result, targetFullDeltaVector);
	}

	void collateDeltaRisk(const LTQC::Matrix &targetA, const LTQC::Matrix &depTargetA, const std::vector<double>& sourceDeltaRisk,const std::vector<double>& sourceSpreadRisk, BaseModelPtr sourceModel, BaseModelPtr depSourceModel, BaseModelPtr targetModel, FlexYCF::InstrumentDeltaVector& targetDeltaVector)
	{
	
		CachedDerivInstruments sourcePartialInstruments(sourceModel->getFullPrecomputedInstruments());
		CachedDerivInstruments depSourcePartialInstruments(depSourceModel->getFullPrecomputedInstruments());
		if(sourceDeltaRisk.size() + sourceSpreadRisk.size() != sourcePartialInstruments.size() + depSourcePartialInstruments.size())
		{
			LTQC_THROW(SystemException, "Size of source risk and the number of source yield curve (dep yield curve) don't match: risk " << sourceDeltaRisk.size() <<" + "<<  sourceSpreadRisk.size() << ", source curves " << sourcePartialInstruments.size() << " + " << depSourcePartialInstruments.size());
		}
		vector<double> sourceRiskPlacedInstruments, depSourceRiskPlacedInstruments;
		placedInstrumentsWeights(sourceModel, sourceSpreadRisk, sourceRiskPlacedInstruments);
		placedInstrumentsWeights(depSourceModel, sourceDeltaRisk, depSourceRiskPlacedInstruments);
			
		VectorDouble result1, result2, result, sourceRisk(depSourceRiskPlacedInstruments.begin(),depSourceRiskPlacedInstruments.end()), sourceSpread(sourceRiskPlacedInstruments.begin(),sourceRiskPlacedInstruments.end());
		//weightsFromRisk(depSourceModel, sourceRisk);
		//weightsFromRisk(sourceModel, sourceSpread);

        dot_and_assign(depTargetA, sourceRisk, false, result1);
		dot_and_assign(targetA, sourceSpread, false, result2);
        result.resize(result1.size());
		for(size_t i = 0; i < result1.size(); ++i)
		{
			result[i] = result1[i] + result2[i];
		}

		riskFromWeights(targetModel, result, targetDeltaVector);
	}

	void collateDeltaDepRisk(const LTQC::Matrix &target, const LTQC::Matrix &depTarget,  const LTQC::Matrix &dep2Target, const std::vector<double>& sourceDepDeltaRisk, const std::vector<double>& sourceDep2DeltaRisk, const std::vector<double>& sourceSpreadRisk, BaseModelPtr sourceModel, BaseModelPtr depSourceModel,  BaseModelPtr dep2SourceModel, 
		BaseModelPtr targetModel1,  BaseModelPtr targetModel2, FlexYCF::InstrumentDeltaVector& targetDeltaVector1, FlexYCF::InstrumentDeltaVector& targetDeltaVector2)
	{
		
		vector<double> sourceRiskPlacedInstruments, depSourceRiskPlacedInstruments, dep2SourceRiskPlacedInstruments;
		placedInstrumentsWeights(sourceModel, sourceSpreadRisk, sourceRiskPlacedInstruments);
		placedInstrumentsWeights(depSourceModel, sourceDepDeltaRisk, depSourceRiskPlacedInstruments);
		placedInstrumentsWeights(dep2SourceModel, sourceDep2DeltaRisk, dep2SourceRiskPlacedInstruments);

		

		VectorDouble resultDep, resultDep2, resultSpread, dep, dep2,  sourceDepRisk(depSourceRiskPlacedInstruments.begin(),depSourceRiskPlacedInstruments.end()), sourceDep2Risk(dep2SourceRiskPlacedInstruments.begin(),dep2SourceRiskPlacedInstruments.end()), sourceSpread(sourceRiskPlacedInstruments.begin(),sourceRiskPlacedInstruments.end());
		

        dot_and_assign(depTarget, sourceDepRisk, false, resultDep);
		dot_and_assign(dep2Target, sourceDep2Risk, false, resultDep2);
		dot_and_assign(target, sourceSpread, false, resultSpread);
        
		dep.resize(depSourceRiskPlacedInstruments.size());
		for(size_t i = 0; i < dep.size(); ++i)
		{
			dep[i] = resultSpread[i] + resultDep[i] +  resultDep2[i];
		}
		dep2.resize(dep2SourceRiskPlacedInstruments.size());
		size_t offset = dep.size();
		for(size_t i = 0; i < dep2.size(); ++i)
		{
			dep2[i] = resultSpread[i + offset] + resultDep[i + offset] +  resultDep2[i + offset];
		}
		std::vector<double> targetDeltaRisk;
		riskFromWeights(targetModel1,dep,targetDeltaVector1);
		riskFromWeights(targetModel2,dep2,targetDeltaVector2);
	}


	void transitionMatrixSourceToDepTarget(IDeA::YieldCurveIFConstPtr  targetYieldCurve, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr sourceMasterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, BaseModelPtr riskInstModel, IDeA::AssetDomainConstPtr depAssetDomain, LTQC::Matrix& matrix)
	{
		const FlexYCF::CalibrationInstruments& sourceFullInstruments = sourceModel->getFullPrecomputedInstruments().calibrationInstruments();
        
		CachedDerivInstruments sourcePartialInstruments(sourceModel->getFullPrecomputedInstruments());
		CachedDerivInstruments targetPartialInstruments(riskInstModel->getFullPrecomputedInstruments());
		size_t noCols = sourceModel->numberOfPlacedInstruments();
		
		size_t noRows = riskInstModel->numberOfPlacedInstruments();
		matrix.resize(noRows , noCols);
		
		std::map<LT::Str, double> fxRates;
		fxRates[assetDomain->primaryDomain()] = 1.0;
		fxRates[depAssetDomain->primaryDomain()] = 1.0;

		for(size_t i = 0, j = 0; j < sourcePartialInstruments.size(); ++j)
		{
			if( sourcePartialInstruments[j]->wasPlaced() )
			{
				RepFlowsData<IDeA::Funding> fundingRepFlows;
				RepFlowsData<IDeA::Index> indexRepFlows;
				(sourceFullInstruments)[j]->fillRepFlows(assetDomain, *targetModel,1.0, indexRepFlows);
				(sourceFullInstruments)[j]->fillRepFlows(assetDomain, *targetModel,1.0, fundingRepFlows);

			
				FlexYCF::InstrumentDeltaVector fullDeltaVector;
				vector<YieldCurveIFConstPtr> childModel;
				childModel.push_back(targetYieldCurve);

				vector<LT::Str> childModelAD;
				childModelAD.push_back(assetDomain->discriminator());

				fullDeltaVector = FlexYCF::calculateRisk(riskInstModel, depAssetDomain, childModel, childModelAD, fundingRepFlows, indexRepFlows,fxRates);
				for(size_t k = 0, n=0; k < fullDeltaVector.size(); ++k )
				{
					if( targetPartialInstruments[k]->wasPlaced() )
					{
						matrix(n,i) = - fullDeltaVector[k].getHedgeRatio();
						++n;
					}
				}
				++i;
			}
		}
	}

	void transitionMatrixSourceToDepTarget2(IDeA::YieldCurveIFConstPtr  targetYieldCurve, IDeA::YieldCurveIFConstPtr  targetChildYieldCurve, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr sourceMasterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, std::vector<BaseModelPtr> depTargetModel, std::vector<IDeA::AssetDomainConstPtr> depAssetDomain, LTQC::Matrix& matrix)
	{
		const FlexYCF::CalibrationInstruments& sourceFullInstruments = sourceModel->getFullPrecomputedInstruments().calibrationInstruments();
        
		CachedDerivInstruments sourcePartialInstruments(sourceModel->getFullPrecomputedInstruments());
		CachedDerivInstruments targetPartialInstruments(depTargetModel[0]->getFullPrecomputedInstruments());
		size_t noCols = sourceModel->numberOfPlacedInstruments();
		
		size_t noRows = depTargetModel[0]->numberOfPlacedInstruments();
		matrix.resize(noRows , noCols);
		
		std::map<LT::Str, double> fxRates;
		fxRates[assetDomain->primaryDomain()] = 1.0;
		fxRates[depAssetDomain[0]->primaryDomain()] = 1.0;

		for(size_t i = 0, j = 0; j < sourcePartialInstruments.size(); ++j)
		{
			if( sourcePartialInstruments[j]->wasPlaced() )
			{
				RepFlowsData<IDeA::Funding> fundingRepFlows;
				RepFlowsData<IDeA::Index> indexRepFlows;
				(sourceFullInstruments)[j]->fillRepFlows(depAssetDomain[0], *depTargetModel[0],1.0, indexRepFlows);
				(sourceFullInstruments)[j]->fillRepFlows(depAssetDomain[0], *depTargetModel[0],1.0, fundingRepFlows);

			
			
				FlexYCF::InstrumentDeltaVector fullDeltaVector;
				vector<YieldCurveIFConstPtr> childModel;
				childModel.push_back(targetYieldCurve);

				vector<LT::Str> childModelAD;
				childModelAD.push_back(assetDomain->discriminator());

				fullDeltaVector = FlexYCF::calculateRisk(depTargetModel[0], depAssetDomain[0], childModel, childModelAD, fundingRepFlows, indexRepFlows,fxRates);
				
				for(size_t k = 0, n=0; k < fullDeltaVector.size(); ++k )
				{
					if( targetPartialInstruments[k]->wasPlaced() )
					{
						matrix(n,i) = - fullDeltaVector[k].getHedgeRatio();
						++n;
					}
				}
				++i;
			}
		}

		if(depTargetModel.size() == 2)
		{
			CachedDerivInstruments targetPartialInstruments(depTargetModel[1]->getFullPrecomputedInstruments());
			
			size_t offset = noRows;
			size_t noRows1 = depTargetModel[1]->numberOfPlacedInstruments();
			matrix.resize(noRows + noRows1, noCols);
		
			std::map<LT::Str, double> fxRates;
			fxRates[assetDomain->primaryDomain()] = 1.0;
			fxRates[depAssetDomain[0]->primaryDomain()] = 1.0;
			fxRates[depAssetDomain[1]->primaryDomain()] = 1.0;
			vector<YieldCurveIFConstPtr> childModel;
			childModel.push_back(targetChildYieldCurve);	
			childModel.push_back(targetYieldCurve);

					
			vector<LT::Str> childModelAD;
			childModelAD.push_back(depAssetDomain[0]->discriminator());		
			childModelAD.push_back(assetDomain->discriminator());
			
			for(size_t i = 0, j = 0; j < sourcePartialInstruments.size(); ++j)
			{
				if( sourcePartialInstruments[j]->wasPlaced() )
				{
					RepFlowsData<IDeA::Funding> fundingRepFlows;
					RepFlowsData<IDeA::Index> indexRepFlows;
					(sourceFullInstruments)[j]->fillRepFlows(assetDomain, *targetModel,1.0, indexRepFlows);
					(sourceFullInstruments)[j]->fillRepFlows(assetDomain, *targetModel,1.0, fundingRepFlows);

			
					ReplicatingFlows<IDeA::Funding> fRepFlows( fundingRepFlows.toRepFlows( depTargetModel[1]->getValueDate(), assetDomain ) );
					ReplicatingFlows<IDeA::Index> iRepFlows( indexRepFlows.toRepFlows( depTargetModel[1]->getValueDate(), assetDomain ) );
			
					FlexYCF::InstrumentDeltaVector fullDeltaVector;
					

					fullDeltaVector = FlexYCF::calculateRisk(depTargetModel[1], depAssetDomain[1], childModel, childModelAD, fundingRepFlows, indexRepFlows,fxRates);
				
					for(size_t k = 0, n=0; k < fullDeltaVector.size(); ++k )
					{
						if( targetPartialInstruments[k]->wasPlaced() )
						{
							matrix(n + offset,i) = - fullDeltaVector[k].getHedgeRatio();
							++n;
						}
					}
					++i;
				}
			}
		}
	}


	void transitionMatrixSourceToTarget(IDeA::YieldCurveIFConstPtr  targetYieldCurve, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr sourceMasterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, LTQC::Matrix& matrix)
	{
		const FlexYCF::CalibrationInstruments& sourceFullInstruments = sourceModel->getFullPrecomputedInstruments().calibrationInstruments();
        
		CachedDerivInstruments sourcePartialInstruments(sourceModel->getFullPrecomputedInstruments());
		CachedDerivInstruments targetPartialInstruments(targetModel->getFullPrecomputedInstruments());
		size_t noCols = sourceModel->numberOfPlacedInstruments();
		
		size_t noRows = targetModel->numberOfPlacedInstruments();
		matrix.resize(noRows , noCols);
		
		for(size_t i = 0, j = 0; j < sourcePartialInstruments.size(); ++j)
		{
			if( sourcePartialInstruments[j]->wasPlaced() )
			{
				RepFlowsData<IDeA::Funding> fundingRepFlows;
				RepFlowsData<IDeA::Index> indexRepFlows;
				(sourceFullInstruments)[j]->fillRepFlows(assetDomain, *targetModel,1.0, indexRepFlows);
				(sourceFullInstruments)[j]->fillRepFlows(assetDomain, *targetModel,1.0, fundingRepFlows);
				
				FlexYCF::InstrumentDeltaVector fullDeltaVector;
				ReplicatingFlows<IDeA::Funding> fRepFlows( fundingRepFlows.toRepFlows( targetModel->getValueDate( ), assetDomain ) );
				ReplicatingFlows<IDeA::Index> iRepFlows( indexRepFlows.toRepFlows( targetModel->getValueDate( ), assetDomain ) );
				FlexYCF::calculateIRAnalyticalDelta( *targetModel, fRepFlows, iRepFlows, fullDeltaVector );
				
				for(size_t k = 0, n=0; k < fullDeltaVector.size(); ++k )
				{
					if( targetPartialInstruments[k]->wasPlaced() )
					{
						matrix(n,i) = - fullDeltaVector[k].getHedgeRatio();
						++n;
					}
				}
				++i;
			}
		}
	}

	void transitionMatrixDepToDep(LT::TablePtr sourceMasterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, BaseModelPtr riskInstModel, IDeA::AssetDomainConstPtr assetDomain, LTQC::Matrix& matrix)
	{
		const FlexYCF::CalibrationInstruments& sourceFullInstruments = sourceModel->getFullPrecomputedInstruments().calibrationInstruments();
        
		CachedDerivInstruments sourcePartialInstruments(sourceModel->getFullPrecomputedInstruments());
		CachedDerivInstruments targetPartialInstruments(riskInstModel->getFullPrecomputedInstruments());
		size_t noCols = sourceModel->numberOfPlacedInstruments();
		
		size_t noRows = riskInstModel->numberOfPlacedInstruments();
		matrix.resize(noRows , noCols);
		
		std::map<LT::Str, double> fxRates;
		fxRates[assetDomain->primaryDomain()] = 1.0;

		for(size_t i = 0, j = 0; j < sourcePartialInstruments.size(); ++j)
		{
			if( sourcePartialInstruments[j]->wasPlaced() )
			{
				RepFlowsData<IDeA::Funding> fundingRepFlows;
				RepFlowsData<IDeA::Index> indexRepFlows;
				(sourceFullInstruments)[j]->fillRepFlows(assetDomain, *targetModel,1.0, indexRepFlows);
				(sourceFullInstruments)[j]->fillRepFlows(assetDomain, *targetModel,1.0, fundingRepFlows);

			
				ReplicatingFlows<IDeA::Funding> fRepFlows( fundingRepFlows.toRepFlows( riskInstModel->getValueDate( ), assetDomain ) );
				ReplicatingFlows<IDeA::Index> iRepFlows( indexRepFlows.toRepFlows( riskInstModel->getValueDate( ), assetDomain ) );
			
				FlexYCF::InstrumentDeltaVector fullDeltaVector;
				vector<YieldCurveIFConstPtr> childModel;
				vector<LT::Str> childModelAD;
				fullDeltaVector = FlexYCF::calculateRisk(riskInstModel, assetDomain, childModel, childModelAD, fundingRepFlows, indexRepFlows,fxRates);
				
				for(size_t k = 0, n=0; k < fullDeltaVector.size(); ++k )
				{
					if( targetPartialInstruments[k]->wasPlaced() )
					{
						matrix(n,i) = - fullDeltaVector[k].getHedgeRatio();
						++n;
					}
				}
				++i;
			}
		}



	}

	void transitionMatrixDepToDep2(IDeA::AssetDomainConstPtr ad, IDeA::YieldCurveIFConstPtr  targetYieldCurve, IDeA::YieldCurveIFConstPtr  targetChildYieldCurve, LT::TablePtr sourceMasterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, std::vector<BaseModelPtr> depTargetModel, std::vector<IDeA::AssetDomainConstPtr> assetDomain, LTQC::Matrix& matrix)
	{
		const FlexYCF::CalibrationInstruments& sourceFullInstruments = sourceModel->getFullPrecomputedInstruments().calibrationInstruments();
        
		CachedDerivInstruments sourcePartialInstruments(sourceModel->getFullPrecomputedInstruments());
		CachedDerivInstruments targetPartialInstruments(depTargetModel[0]->getFullPrecomputedInstruments());
		size_t noCols = sourceModel->numberOfPlacedInstruments();
		
		size_t noRows = depTargetModel[0]->numberOfPlacedInstruments();
		matrix.resize(noRows , noCols);
		
		std::map<LT::Str, double> fxRates;
		fxRates[assetDomain[0]->primaryDomain()] = 1.0;

		for(size_t i = 0, j = 0; j < sourcePartialInstruments.size(); ++j)
		{
			if( sourcePartialInstruments[j]->wasPlaced() )
			{
				RepFlowsData<IDeA::Funding> fundingRepFlows;
				RepFlowsData<IDeA::Index> indexRepFlows;
				(sourceFullInstruments)[j]->fillRepFlows(assetDomain[0], *targetModel,1.0, indexRepFlows);
				(sourceFullInstruments)[j]->fillRepFlows(assetDomain[0], *targetModel,1.0, fundingRepFlows);

			
				ReplicatingFlows<IDeA::Funding> fRepFlows( fundingRepFlows.toRepFlows( depTargetModel[0]->getValueDate( ), assetDomain[0] ) );
				ReplicatingFlows<IDeA::Index> iRepFlows( indexRepFlows.toRepFlows( depTargetModel[0]->getValueDate( ), assetDomain[0] ) );
			
				FlexYCF::InstrumentDeltaVector fullDeltaVector;
				vector<YieldCurveIFConstPtr> childModel;
				vector<LT::Str> childModelAD;
				fullDeltaVector = FlexYCF::calculateRisk(depTargetModel[0], assetDomain[0], childModel, childModelAD, fundingRepFlows, indexRepFlows,fxRates);
				
				for(size_t k = 0, n=0; k < fullDeltaVector.size(); ++k )
				{
					if( targetPartialInstruments[k]->wasPlaced() )
					{
						matrix(n,i) = - fullDeltaVector[k].getHedgeRatio();
						++n;
					}
				}
				++i;
			}
		}

	if(depTargetModel.size() == 2)
		{
			CachedDerivInstruments targetPartialInstruments(depTargetModel[1]->getFullPrecomputedInstruments());
			
			size_t offset = noRows;
			size_t noRows1 = depTargetModel[1]->numberOfPlacedInstruments();
			matrix.resize(noRows + noRows1, noCols);
		
			std::map<LT::Str, double> fxRates;
			fxRates[assetDomain[0]->primaryDomain()] = 1.0;
			fxRates[assetDomain[1]->primaryDomain()] = 1.0;

			vector<YieldCurveIFConstPtr> childModel;
			childModel.push_back(targetChildYieldCurve);	
			childModel.push_back(targetYieldCurve);

					
			vector<LT::Str> childModelAD;
			childModelAD.push_back(assetDomain[0]->discriminator());		
			childModelAD.push_back(ad->discriminator());
			
			for(size_t i = 0, j = 0; j < sourcePartialInstruments.size(); ++j)
			{
				if( sourcePartialInstruments[j]->wasPlaced() )
				{
					RepFlowsData<IDeA::Funding> fundingRepFlows;
					RepFlowsData<IDeA::Index> indexRepFlows;
					(sourceFullInstruments)[j]->fillRepFlows(ad, *targetModel,1.0, indexRepFlows);
					(sourceFullInstruments)[j]->fillRepFlows(ad, *targetModel,1.0, fundingRepFlows);

			
					ReplicatingFlows<IDeA::Funding> fRepFlows( fundingRepFlows.toRepFlows( depTargetModel[1]->getValueDate(), assetDomain[1] ) );
					ReplicatingFlows<IDeA::Index> iRepFlows( indexRepFlows.toRepFlows( depTargetModel[1]->getValueDate(), assetDomain[1] ) );
			
					FlexYCF::InstrumentDeltaVector fullDeltaVector;
					

					fullDeltaVector = FlexYCF::calculateRisk(depTargetModel[1], assetDomain[1], childModel, childModelAD, fundingRepFlows, indexRepFlows,fxRates);
				
					for(size_t k = 0, n=0; k < fullDeltaVector.size(); ++k )
					{
						if( targetPartialInstruments[k]->wasPlaced() )
						{
							matrix(n + offset,i) = - fullDeltaVector[k].getHedgeRatio();
							++n;
						}
					}
					++i;
				}
			}
		}
	}

}