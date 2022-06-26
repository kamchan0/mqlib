/*****************************************************************************
    
	Implementation of StripperModel

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h" 

//	FlexYCF
#include "StripperModel.h"
#include "TenorUtils.h"
#include "UkpCurve.h"
#include "StripperAllDatesKpp.h"
#include "SolverVariable.h"
#include "CurveFormulationFactory.h"
#include "CurveFormulation.h"
#include "MinusLogDiscountCurve.h"
#include "CompositeCurveSeparationUtils.h"
#include "FlexYCFCloneLookup.h"

//	LTQuantLib
#include "Data\GenericData.h"
#include "Maths\Problem.h"

//	IDeA
#include "DictYieldCurve.h"
#include "DataExtraction.h"
#include "SpineDataCache.h"


using namespace LTQC;
using namespace LTQuant;
using namespace std;

namespace FlexYCF
{

    StripperModel::StripperModel(const LT::date valueDate,
                                 const string tenorDescription,
                                 const LTQuant::GenericDataPtr masterTable,
                                 const FlexYCFZeroCurvePtr parent) :
        SingleCurveModel(*masterTable, parent),
        m_tenor(tenorDescToYears(tenorDescription))
    {
        // Retrieve the curve type to use for the stripper by looking inside "Curves Interpolation" table
        //  whether a curve with the right tenor description points to a table with a "Curve Type" tag
        const string defaultCurveTypeName(UkpCurve::getName());
        string curveTypeName(defaultCurveTypeName);

        const string defaultCurveFormulationName(MinusLogDiscountCurve::getName());
        string curveFormulationName(defaultCurveFormulationName);
		const GenericDataPtr modelParametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE,FLEXYC_MODELPARAMETERS)));

        LTQuant::GenericDataPtr curvesInterpTable;
        
        if(modelParametersTable->doesTagExist(IDeA_PARAM(FLEXYC_MODELPARAMETERS, CURVESINTERPOLATION)))
        {
            curvesInterpTable = IDeA::extract<GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, CURVESINTERPOLATION));
            if(curvesInterpTable->doesTagExist(tenorDescription))
            {
                LTQuant::GenericDataPtr tenorCurveTable(curvesInterpTable->get<LTQuant::GenericDataPtr>(tenorDescription, 0));
                
                // Retrieve the curve type
                tenorCurveTable->permissive_get<string>("Curve Type", 0, curveTypeName, defaultCurveTypeName);
                        
                // Retrieve the curve formulation
                tenorCurveTable->permissive_get<string>("Formulation", 0, curveFormulationName, defaultCurveFormulationName);
            }
        }

        m_curveFormulation = CurveFormulationFactory::createInstance(curveFormulationName, 
                                                                     curvesInterpTable,
                                                                     tenorDescription,
                                                                     getLeastSquaresResiduals());
        // possible fixed knot-points (depending on the curve formulation) are
        // added if missing at the CurveFormulation::onFinalize level        
    }

    string StripperModel::getName()
    {
        return "Stripper";
    }

    /// after calib is finished destory all of the calibration
    /// instruments so they do not take up memory via the CachedInstrument component
    /// there are no instruments in this class
    void StripperModel::finishCalibration()
    {
        SingleCurveModel::finishCalibration();
        for_each(m_curveFormulation->begin(),m_curveFormulation->end(),[&](const KnotPoint& kp)
        {
            if(kp.getRelatedInstrument())
            {
                kp.getRelatedInstrument()->finishCalibration(BaseModelPtr(this,NullDeleter()));
            }
        });
    }
    BaseModelPtr StripperModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
         // Retrieve the value date from the masterTable
		const GenericDataPtr detailsTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
		const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        return BaseModelPtr(new StripperModel(valueDate, "3M", masterTable, parent));
    }

    double StripperModel::getTenor() const
    {
        return m_tenor;
    }

    void StripperModel::onKnotPointsPlaced()
    {
		//	Set the separation point at the end of last futures (should be optional)
		if(m_curveFormulation)
		{
			setSeparationPoint(getValueDate(), *m_curveFormulation);
		}
    }

    double StripperModel::getDiscountFactor(const double flowTime) const
    {
		const double structure_(StructureSurfaceHolder::holdee().getDiscountFactor(flowTime));
        return structure_ * m_curveFormulation->getDiscountFactor(flowTime);
    }
    
    double StripperModel::getTenorDiscountFactor(const double flowTime, 
                                                 const double /* tenor */) const
    {
        const double structure_(StructureSurfaceHolder::holdee().getDiscountFactor(flowTime));
        return structure_ * m_curveFormulation->getDiscountFactor(flowTime);
    }

    void StripperModel::accumulateDiscountFactorGradient(const double flowTime, 
                                                         double multiplier,
                                                         GradientIterator gradientBegin,
                                                         GradientIterator gradientEnd) const
    {
        multiplier *= StructureSurfaceHolder::holdee().getDiscountFactor(flowTime);
        m_curveFormulation->accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }

    void StripperModel::accumulateTenorDiscountFactorGradient(const double flowTime, 
                                                              const double /* tenor */, 
                                                              double multiplier, 
                                                              GradientIterator 
                                                              gradientBegin, 
                                                              GradientIterator gradientEnd) const
    {
        multiplier *= StructureSurfaceHolder::holdee().getDiscountFactor(flowTime);
        m_curveFormulation->accumulateDiscountFactorGradient(flowTime, multiplier, gradientBegin, gradientEnd);
    }

	double StripperModel::getVariableValueFromSpineDiscountFactor(const double flowTime,
															 const double discountFactor) const
	{
		const double varValue( m_curveFormulation->getVariableValueFromSpineDiscountFactor(flowTime, discountFactor));
		LT_LOG << "getting var. value: " << varValue << " from df: " << discountFactor << "@ time: " << flowTime << std::endl;
		return varValue;
	}

    std::ostream& StripperModel::print(std::ostream& out) const
    {
       // out << "Stripper model forward rates, tenor:\t" << m_tenor << endl;
        out << "KnotPoints X's:";

        /*  Commented for now
        BaseCurve::const_iterator iter, iter0;
        iter = m_baseCurve->begin();
        for(; iter != m_baseCurve->end(); ++iter)
        {
            out << "\t" << iter->x; 
        }

        out << endl << "Flat inst. fwds\t";
        iter0 = m_baseCurve->begin(); 
        for(iter = iter0 + 1; iter != m_baseCurve->end(); ++iter)
        {
            out << "\t" << (iter->y - iter0->y) / (iter->x - iter0->x);
            iter0 = iter;
        }
        out << endl;
        */
        return out;
    }
    
    void StripperModel::addKnotPoint(const KnotPoint& knotPoint) const
    {
        m_curveFormulation->addKnotPoint(knotPoint);
    }
    
    void StripperModel::setSpotRate(double time, double rate)
    {
        m_curveFormulation->setSpotRate(time,rate);
    }
   
    void StripperModel::update()
    {
        m_curveFormulation->update();
    }

    void StripperModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem)
    {
        m_curveFormulation->addUnknownsToProblem(problem);
    }

	void StripperModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
											  IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		m_curveFormulation->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
	}

	void StripperModel::updateVariablesFromShifts(const LTQC::VectorDouble& variableShifts)
	{
		//	TODO: check # variable shifts = # variables
		m_curveFormulation->updateVariablesFromShifts(variableShifts);
	}

    void StripperModel::finalize()
    {
        m_curveFormulation->finalize();
    }

    void StripperModel::initializeKnotPoints()
    {
        m_curveFormulation->initializeKnotPoints();
    }

	void StripperModel::onInitialized()
	{
		m_curveFormulation->onKnotPointsInitialized();
	}

	LTQuant::GenericDataPtr StripperModel::getSpineCurvesDetails() const
	{
		LTQuant::GenericDataPtr spineCurvesDetailsData(new LTQuant::GenericData("Stripper Spine Curves", 0));
		
		// set curve formulation
		spineCurvesDetailsData->set<LTQuant::GenericDataPtr>("Base Curve", 0, m_curveFormulation->getSpineCurveDetails());
		
		// set structure
		spineCurvesDetailsData->set<LTQuant::GenericDataPtr>("Structure", 0, StructureSurfaceHolder::holdee().getCurveDetails());

		return spineCurvesDetailsData;
	}

	void StripperModel::getSpineInternalData(SpineDataCachePtr& sdp) const {
		m_curveFormulation->getCurveInternalData(sdp->xy_);
	}

	void StripperModel::assignSpineInternalData(SpineDataCachePtr& sdp) {
		m_curveFormulation->assignCurveInternalData(sdp->xy_.begin());
	}

    void StripperModel::getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const
    {
        m_curveFormulation->getSpineCurvesUnfixedKnotPoints(points);
        StructureSurfaceHolder::holdee().getUnfixedKnotPoints(points);
    }

	std::vector<double> StripperModel::abscissas() const
	{
		return m_curveFormulation->abscissas();
	}

    /**
        @brief Create a clone.

        Uses a lookup table to ensure the directed graph relationships are maintained.

        @param lookup A lookup of previously created clones.
        @return       A clone of this instance.
    */
    ICloneLookupPtr StripperModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new StripperModel(*this, lookup));
    }

    /**
        @brief A pseudo copy constructor.

        Uses a lookup table to ensure the directed graph relationships are maintained.

        @param original The instance to be copied.
        @param lookup   A lookup of previously created clones.
    */
    StripperModel::StripperModel(StripperModel const& original, CloneLookup& lookup) :
        SingleCurveModel(original, lookup),
        m_curveFormulation(lookup.get(original.m_curveFormulation)),
        m_tenor(original.m_tenor)
    {
    }
}   //  FlexYCF