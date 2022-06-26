#ifndef __LIBRARY_PRICERS_FLEXYCF_EQUIVALENTIRRISK_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_EQUIVALENTIRRISK_H_INCLUDED

#include "LTQuantInitial.h"
#include "AssetDomain.h"
#include "YieldCurveIF.h"
#include "CachedDerivInstruments.h"
#include "InstrumentDelta.h"
#include "RiskReportFactory.h"


namespace IDeA
{
	class YieldCurveIF;
}

namespace  FlexYCF
{

	class CalibrationInstruments;
	FWD_DECLARE_SMART_PTRS( BaseModel );
	
	void calculateEquivalentIRRisk(LT::TablePtr sourceMasterTable, LT::TablePtr targetMasterTable, const IDeA::YieldCurveIFConstPtr& sourceYieldCurve,
								  const IDeA::YieldCurveIFConstPtr&  targetYieldCurve,
								  const std::vector<double>& sourceDeltaRisk,
								  const std::vector<double>& sourceSpreadDeltaRisk,
								  const std::vector<RiskReportFactory::IRRisk>& fullRisk,
								  IDeA::AssetDomainConstPtr assetDomain,
								  LT::TablePtr& irdeltaTbl);

	void calculateTransitionMatrix(LT::TablePtr sourceMasterTable, LT::TablePtr targetMasterTable, const IDeA::YieldCurveIFConstPtr& sourceYieldCurve, const IDeA::YieldCurveIFConstPtr&  targetYieldCurve, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr& tm);

	void transitionMatrixSourceToDepTarget(IDeA::YieldCurveIFConstPtr  targetYieldCurve, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr masterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, BaseModelPtr riskInstModel, IDeA::AssetDomainConstPtr assetDomain2, LTQC::Matrix& A);
	void transitionMatrixSourceToDepTarget2(IDeA::YieldCurveIFConstPtr  targetYieldCurve, IDeA::YieldCurveIFConstPtr  targetChildYieldCurve, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr masterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, std::vector<BaseModelPtr> riskInstModel, std::vector<IDeA::AssetDomainConstPtr> assetDomain2, LTQC::Matrix& A);


	void transitionMatrixSourceToTarget(IDeA::YieldCurveIFConstPtr  targetYieldCurve, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr masterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, LTQC::Matrix& A);
	void transitionMatrixDepToDep(LT::TablePtr masterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, BaseModelPtr riskInstModel, IDeA::AssetDomainConstPtr assetDomain, LTQC::Matrix& A);
	void transitionMatrixDepToDep2(IDeA::AssetDomainConstPtr ad, IDeA::YieldCurveIFConstPtr  targetYieldCurve, IDeA::YieldCurveIFConstPtr  targetChildYieldCurve, LT::TablePtr sourceMasterTable, BaseModelPtr sourceModel, BaseModelPtr targetModel, std::vector<BaseModelPtr> depTargetModel, std::vector<IDeA::AssetDomainConstPtr> assetDomain, LTQC::Matrix& matrix);


	void collateRisk(const LTQC::Matrix& A, const std::vector<double>& sourceDeltaRisk,BaseModelPtr sourceModel, BaseModelPtr targetModel, FlexYCF::InstrumentDeltaVector& targetFullDeltaVector);
	
	void collateDeltaRisk(const LTQC::Matrix &targetA, const LTQC::Matrix &depTargetA, const std::vector<double>& sourceDeltaRisk,const std::vector<double>& sourceSpreadRisk, BaseModelPtr sourceModel, BaseModelPtr depSourceModel, BaseModelPtr targetModel, FlexYCF::InstrumentDeltaVector& targetDeltaVector);
	
	void getDependentModels(LT::TablePtr masterTable, std::vector<IDeA::AssetDomainConstPtr>& assetDomains, std::vector<LT::TablePtr>& depMasterTables);
	void dependentModel(LT::TablePtr masterTable, IDeA::AssetDomainConstPtr assetDomain, LT::TablePtr& depMasterTable);

	void placedInstrumentsWeights(BaseModelPtr model, const std::vector<double>& inputRisk, std::vector<double>& outputRisk);
	
	void collateDeltaDepRisk(const LTQC::Matrix &target, const LTQC::Matrix &depTarget,  const LTQC::Matrix &dep2Target, const std::vector<double>& sourceDepDeltaRisk, const std::vector<double>& sourceDep2DeltaRisk, const std::vector<double>& sourceSpreadRisk, BaseModelPtr sourceModel, BaseModelPtr depSourceModel,  BaseModelPtr dep2SourceModel, 
		BaseModelPtr targetModel1,  BaseModelPtr targetModel2, FlexYCF::InstrumentDeltaVector& targetDeltaVector1, FlexYCF::InstrumentDeltaVector& targetDeltaVector2);

	/*void transitionMatrix(LT::TablePtr sourceMasterTable, std::vector<LT::TablePtr> depSourceMasterTables, BaseModelPtr sourceModel, std::vector<BaseModelPtr> depSourceModel, std::vector<IDeA::AssetDomainConstPtr> depSourceModelAD, 
		BaseModelPtr targetModel, std::vector<BaseModelPtr> depTargetModel, IDeA::AssetDomainConstPtr assetDomain, LTQC::Matrix& matrix);*/
}


#endif 