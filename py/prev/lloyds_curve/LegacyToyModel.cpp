/*****************************************************************************

    The FlexYCF single curve model.
    
    @Originator		Justin Ware
    
    Copyright (C) Lloyds Banking Group 2010 All Rights Reserved

*****************************************************************************/

#include "stdafx.h"

// FlexYCF
#include "LegacyToyModel.h"


#include "Exception.h"
#include "QCException.h"


using namespace std;


using namespace LTQC;

namespace FlexYCF
{    
    using namespace LTQuant;
	/**
		@brief Pseudo copy constructor.

		The BaseModel needs to see CloneLookup to clone properly.

		@param original The original instance to be copied.
		@param lookup   A lookup of previously created clones.
	*/
	LegacyToyModel::LegacyToyModel(LegacyToyModel const& original, CloneLookup& lookup) : 
		BaseModel(original, lookup)
	{
	}

	LegacyToyModel::LegacyToyModel(IDeA::ZeroCurveIFPtr zc) : 
		m_zc(zc)
	{
		JacobianIsNotSupported();
	}

	ICloneLookupPtr LegacyToyModel::cloneWithLookup(CloneLookup& lookup) const
	{
		return ICloneLookupPtr(new LegacyToyModel(*this, lookup));
	}


    double LegacyToyModel::getDiscountFactor(const double flowTime) const
	{
		return m_zc->getFundingDF2(flowTime);
	}
	double LegacyToyModel::getTenorDiscountFactor(const double flowTime, const double tenor) const
	{
		return m_zc->getIndexDF2(flowTime, tenor);
	}
    double LegacyToyModel::getStructureFactor(const double flowTime) const
    {
        return m_zc->getStructureFactor(flowTime);
    }

	// DISABLED
	// *************************************************************************************************************************************
	void LegacyToyModel::update()
	{
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	}
	void LegacyToyModel::finalize()
	{
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	}
		
	 void LegacyToyModel::addKnotPoint(const KnotPoint& knotPoint) const
	 {
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	 }
	 void LegacyToyModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem)
	 {
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	 }
	void LegacyToyModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
									   	   IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	}
	void LegacyToyModel::updateVariablesFromShifts(const LTQC::VectorDouble& variableShifts)
	{
	}

	double LegacyToyModel::getSpineDiscountFactor(const double flowTime,
										  const CurveTypeConstPtr& /* curveType */) const
	{
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
		return 0.0;
	}

	 void LegacyToyModel::accumulateDiscountFactorGradient(const double flowTime, 
												  double multiplier,
												  GradientIterator gradientBegin,
												  GradientIterator gradientEnd) const
	 {
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	 }

	 void LegacyToyModel::accumulateTenorDiscountFactorGradient(const double flowTime,
													   const double tenor, 
													   double multiplier, 
													   GradientIterator gradientBegin, 
													   GradientIterator gradientEnd) const
	 {
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	 }

	 void LegacyToyModel::accumulateDiscountFactorGradient(const double flowTime, 
												  double multiplier,
												  GradientIterator gradientBegin,
												  GradientIterator gradientEnd,
												  const CurveTypeConstPtr& /* curveType */) const
	{
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	}

	 void LegacyToyModel::accumulateTenorDiscountFactorGradient(const double flowTime,
													   const double tenor, 
													   double multiplier, 
													   GradientIterator gradientBegin, 
													   GradientIterator gradientEnd,
													   const CurveTypeConstPtr& /* curveType */) const
	{
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
	}

	 double LegacyToyModel::getVariableValueFromSpineDiscountFactor(const double flowTime,
														   const double discountFactor) const
	 {
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
		return 0.0;
	 }
	 double LegacyToyModel::getVariableValueFromSpineDiscountFactor(const double flowTime,
														   const double discountFactor,
														   const CurveTypeConstPtr&) const
	{
		LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
		return 0.0;
	 }

     void LegacyToyModel::getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const
     {
         LTQC_THROW(IDeA::MarketException, "Functionality only supported in FlexYCF models. Try changing model type of the yield curve");
     }
     /// after calib is finished destory all of the calibration
     /// instruments so they do not take up memory via the CachedInstrument component
     /// but the LegacyToyModel has no calib instrumrnts that need to be cleaned up
     void LegacyToyModel::finishCalibration()
     {
         BaseModel::finishCalibration();
     }
	// *************************************************************************************************************************************	 
}