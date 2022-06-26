
#include "stdafx.h"


//	FlexYCF
#include "MultiTenorOISModel.h"
#include "CurveType.h"
#include "SolverVariable.h"
#include "InterpolationMethodFactory.h"
#include "ICurveFactory.h"
#include "LeastSquaresResiduals.h"
#include "Gradient.h"
#include "TssInterpolationFactory.h"
#include "CompositeCurveSeparationUtils.h"
#include "FlexYCFCloneLookup.h"
#include "CurveUtils.h"

#include "Maths\Problem.h"
#include "Data\GenericData.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"
#include "SpineDataCache.h"

using namespace std;


using namespace LTQC;

namespace FlexYCF
{    
    using namespace LTQuant;

    MultiTenorOISModel::MultiTenorOISModel(const LT::date valueDate,
                                     const CurveTypeConstPtr& baseRate,
									 const LTQuant::GenericDataPtr& masterTable,
                                     const FlexYCFZeroCurvePtr parent):
        MultiCurveModel(*masterTable, parent),
		m_baseRate(baseRate),
        m_tenorSpreadSurface(baseRate, *masterTable)
    {
    }

    BaseModelPtr MultiTenorOISModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
        const GenericDataPtr modelParametersTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS)));

        LT_LOG << "Creating MultiTenorOIS Model" << std::endl;
         // The following is for DEBUG only
		const GenericDataPtr parametersTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
        string currency("");
		IDeA::permissive_extract<std::string>(*parametersTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency);

		LT_LOG << "currency: " << currency << std::endl;

        // Retrieve the value date
		const GenericDataPtr detailsTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        // Retrieve the base rate
        string baseRateDescription(CurveType::Discount()->getDescription());
		
        const CurveTypeConstPtr baseRate(CurveType::getFromDescription(baseRateDescription));

        // Create the MultiTenorOISModel
        MultiTenorOISModelPtr multiTenorModel(new MultiTenorOISModel(valueDate, baseRate, masterTable, parent));	//	modelParametersTable));

        // Retrieve the "Curves Interpolation" table
        LTQuant::GenericDataPtr curvesInterpolationTable;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, CURVESINTERPOLATION), curvesInterpolationTable);
			
        if(static_cast<bool>(curvesInterpolationTable))
        {
            LTQuant::GenericDataPtr interpolationDetailsTable;
            
            // Create the base rate curve
            curvesInterpolationTable->permissive_get<LTQuant::GenericDataPtr>(baseRate->getDescription(), 0, interpolationDetailsTable, LTQuant::GenericDataPtr());
            multiTenorModel->m_baseRateCurve = ICurveFactory::createInstance(interpolationDetailsTable, multiTenorModel->getLeastSquaresResiduals());
            LT_LOG << "Base Rate: " << baseRate->getDescription() << " - " << (*(multiTenorModel->m_baseRateCurve)) << endl;

            // Create the tenor spread surface curves
            multiTenorModel->m_tenorSpreadSurface.createTenorSpreadCurves(curvesInterpolationTable, multiTenorModel->getLeastSquaresResiduals());
        }
        else
        {
            LT_THROW_ERROR("No 'Curves Interpolation' table provided to build the MultiTenorOISModel.");
        }

        return multiTenorModel;
    }

    CurveTypeConstPtr MultiTenorOISModel::getBaseRate() const
    {
        return m_baseRate;
    }

    double MultiTenorOISModel::getDiscountFactor(const double flowTime) const
    {
        return exp( -( m_baseRateCurve->evaluate(flowTime) + StructureSurfaceHolder::holdee().getLogFvf(flowTime) ) );        
    }

    double MultiTenorOISModel::getTenorDiscountFactor(const double flowTime, const double tenor) const
    {
        // Note the tenor conversion here from a double to one of the 'bucket' CurveType'd tenor
        return exp( -( m_baseRateCurve->evaluate(flowTime) + m_tenorSpreadSurface.interpolateCurve(tenor, flowTime) + StructureSurfaceHolder::holdee().getLogFvf(flowTime) ) );
    }

    // For a point Ri on the base rate curve, the partial derivative dP(t) / dRi can we decomposed as:
    //  dP(t) / dR(t)  x  dR(t) / dRi.  From there, dP(t)/dR(t) = - P(t)  because P(t) = exp{-[R(t) + DiscSpread(t)]}, and can be computed
    //  directly from the model. dR(t) / dRi is computed by a virtual function in the curve class that delegates calculation
    //  of gradient to its interpolation method class.
    //  Note:
    // in the multi-tenors model, the discount factor P(t) is only sensitive to 8 unknowns at most (depending on the interpolation method):
    // the two knot points immediately on the left of t and the two knot points immediately on the right of the base rate curve and the discounted 
    // spread curve.
    void MultiTenorOISModel::accumulateDiscountFactorGradient(const double flowTime,
                                                           double multiplier,
                                                           GradientIterator gradientBegin,
                                                           GradientIterator gradientEnd) const
    {       
        multiplier *= -getDiscountFactor(flowTime);
        // an iterator that keeps track of the position where to 
        // insert the gradients relative to the UKP on each curve
        GradientIterator gradIter = gradientBegin;         
       
        // Compute the gradient relative to the base rate curve knot-points only and copy into the big gradient
        m_baseRateCurve->accumulateGradient(flowTime, multiplier, gradIter, gradIter + m_baseRateCurve->getNumberOfUnknowns());

        gradIter += m_baseRateCurve->getNumberOfUnknowns();  
    }

    void MultiTenorOISModel::accumulateTenorDiscountFactorGradient(const double flowTime, 
                                                                const double tenor,
                                                                double multiplier,
                                                                GradientIterator gradientBegin,
                                                                GradientIterator gradientEnd) const
    {
        // Multiply the calculated gradient by -P(t)
        multiplier *= -getTenorDiscountFactor(flowTime, tenor);
    
        // Compute gradient on base  (done in previous function)
        GradientIterator gradIter = gradientBegin;          // an iterator that keeps track of the position where to insert the smaller gradients
       
        m_baseRateCurve->accumulateGradient(flowTime, multiplier, gradIter, gradIter + m_baseRateCurve->getNumberOfUnknowns());

        gradIter += m_baseRateCurve->getNumberOfUnknowns();  

        // compute the gradient of all unknown knot-points on the tenor spread surface
        m_tenorSpreadSurface.accumulateCurveGradient(tenor, flowTime, multiplier, gradIter, gradIter + m_tenorSpreadSurface.getNumberOfUnknowns());
    }

	void MultiTenorOISModel::accumulateDiscountFactorGradient(const double flowTime, 
														   double multiplier,
														   GradientIterator gradientBegin,
														   GradientIterator gradientEnd,
														   const CurveTypeConstPtr& curveType) const
	{
		// Multiply the calculated gradient by -P(t)
        multiplier *= -getDiscountFactor(flowTime);
    
		if(curveType == m_baseRate)
		{
			m_baseRateCurve->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
		}
	}

	void MultiTenorOISModel::accumulateTenorDiscountFactorGradient(const double flowTime,
															    const double tenor, 
															    double multiplier, 
															    GradientIterator gradientBegin, 
															    GradientIterator gradientEnd,
															    const CurveTypeConstPtr& curveType) const
	{
		// curve type not used here...
		// Multiply the calculated gradient by -P(t)
        multiplier *= -getTenorDiscountFactor(flowTime, tenor);
           
		if(curveType == m_baseRate)
		{
			m_baseRateCurve->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
		}
		else
		{
			// compute the gradient of all unknown knot-points on the tenor spread surface
			// Note: the tenor spread curve on which to interpolate is deduced from tenor,
			// it is NOT the specified curve type
			m_tenorSpreadSurface.accumulateCurveGradient(CurveType::getFromYearFraction(tenor),
														 flowTime,
														 multiplier,
														 gradientBegin,
														 gradientEnd, 
														 curveType);
		}
	}

    void MultiTenorOISModel::onKnotPointsPlaced()
    {	
		//	Limit scope of the cast
		{
			const BaseCurvePtr baseRateCurveAsBaseCurve(std::tr1::dynamic_pointer_cast<BaseCurve>(m_baseRateCurve));
		
			if(baseRateCurveAsBaseCurve)
			{
				//	Set the separation point at the end of last futures (should be optional)
				setSeparationPoint(getValueDate(), *baseRateCurveAsBaseCurve);
			}
		}

        // Could those be moved to finalize?
        m_numberOfBaseRateCurveUnknowns = m_baseRateCurve->getNumberOfUnknowns();
        m_totalNumberOfUnknowns = m_numberOfBaseRateCurveUnknowns + m_tenorSpreadSurface.getNumberOfUnknowns();

    }

    void MultiTenorOISModel::addKnotPoint(const CurveTypeConstPtr curveType, const KnotPoint& knotPoint)
    {
        // add the knot point to the right curve
        if(curveType == m_baseRate)
        {
            m_baseRateCurve->addKnotPoint(knotPoint);
        }
        else 
        {
            if(!m_tenorSpreadSurface.curveExists(curveType))
            {
				LT_THROW_ERROR( std::string("No curve of type '").append(curveType->getDescription()).append(" exists in the tenor spread surface") );
            }
            m_tenorSpreadSurface.addKnotPoint(curveType, knotPoint);
        }
    }

    void MultiTenorOISModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem)
    {
        // 1. add the Unknown Knot-Points (UKP) of the base curve to the problem
        m_baseRateCurve->addUnknownsToProblem(problem);

        // 2. add the UKPs of the various tenor spread curves to the problem
        m_tenorSpreadSurface.addUnknownsToProblem(problem);
    }

	void MultiTenorOISModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
									   			IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		m_baseRateCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem); 
		m_tenorSpreadSurface.addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem, CurveType::AllTenors());
	}

	void MultiTenorOISModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
												const CurveTypeConstPtr& curveType,
												IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		if(curveType == m_baseRate)
		{
			m_baseRateCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
		} 
		else
		{
			// should pass the curve type and add only unknowns on the curve of the specified curve type
			// what we want is to add all unknowns on the spread surface, so for a special curve type
			// the function in tenor spread surface should add everything
			//if(!m_tenorSpreadSurface.curveExists(curveType))
			//{
			//	LT_THROW_ERROR( "No curve with the specified type exists in the tenor spread surface" );
			//}
			m_tenorSpreadSurface.addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem, curveType);
		}
	}

	void MultiTenorOISModel::updateVariablesFromShifts(const LTQC::VectorDouble& variablesShifts)
	{
		//	TODO: check # of variable shifts = # of unknowns

		size_t k(0);

		//	Shift variables on the base spine curve
		size_t numberOfUnknowns(m_baseRateCurve->getNumberOfUnknowns());
		for( ; k < numberOfUnknowns; ++k)
		{
			m_baseRateCurve->shiftUnknown(k, variablesShifts[k]); 
		}

		//	Shift variables on the tenor spread surface
		//	Note: must pass only the appropriate subset of the variables to shift 
		m_tenorSpreadSurface.updateVariablesFromShifts(variablesShifts.begin() + numberOfUnknowns, variablesShifts.end());

        update();
	};

    void MultiTenorOISModel::update()
    {
        // Update the base rate curve
        m_baseRateCurve->update();

        // Update the tenor spread surface
        m_tenorSpreadSurface.update();
    }

    void MultiTenorOISModel::finalize()
    {
		 // Finalize the base rate curve
		finalizeLogFvfCurve(*m_baseRateCurve);

        // Finalize the tenor spread surface
        m_tenorSpreadSurface.finalize();    
    }

	void MultiTenorOISModel::initializeKnotPoints()
	{
		// Default initialization to ease testing
		initializeKnotPoints(m_baseRate, 0.03);
		
		initializeKnotPoints(CurveType::AllTenors(), 0.0);
	}

	void MultiTenorOISModel::initializeKnotPoints(const CurveTypeConstPtr& curveType,
			 								   const double initialSpotRate)
	{
		LT_LOG << "Initializing curve " << curveType->getDescription() << " with spot rate " << initialSpotRate;

		// the function passed relies on logFvf representation
		if(curveType == m_baseRate)
		{	
			m_baseRateCurve->initialize([initialSpotRate] (double x) { return multiTenorInitializationFunc(initialSpotRate, x);});
		}
		else
		{
			m_tenorSpreadSurface.initializeKnotPoints(curveType, initialSpotRate);
		}
	}

	double MultiTenorOISModel::getVariableValueFromSpineDiscountFactor(const double flowTime,
																	const double discountFactor,
																	const CurveTypeConstPtr& /* curveType */) const
	{
		// Note: specifying the curve type will allow the curves of the model
		//	to have different formulations. For now, all formulations are
		//	hardcoded as LogFvf so no use
		double variableValue(-log(discountFactor));

		LT_LOG << "getting var. value: " << variableValue << " from df: " << discountFactor << "@ time: " << flowTime << std::endl;
		
		return variableValue;
	}

	size_t MultiTenorOISModel::getNumberOfUnknowns(const CurveTypeConstPtr& curveType) const
	{
		if(curveType == m_baseRate )
		{
			return m_baseRateCurve->getNumberOfUnknowns();
		}
		else
		{
			return m_tenorSpreadSurface.getNumberOfUnknowns(curveType);
		}
	}

	LTQuant::GenericDataPtr MultiTenorOISModel::getSpineCurvesDetails() const
	{
		const LTQuant::GenericDataPtr spineCurvesDetailsData(new LTQuant::GenericData("Spine Curves Details", 0));
		spineCurvesDetailsData->set<std::string>("Base Rate", 0, m_baseRate->getDescription());
		
		LTQuant::GenericDataPtr spineCurvesData(new LTQuant::GenericData("Spine Curves", 0));
		spineCurvesDetailsData->set<LTQuant::GenericDataPtr>("Curves", 0, spineCurvesData);

		LTQuant::GenericDataPtr spineCurveData;

		//	Base Rate Spine Curve
		spineCurveData = m_baseRateCurve->getCurveDetails();
		spineCurvesData->set<LTQuant::GenericDataPtr>("Base Rate", 0, spineCurveData);
		const size_t nbBaseSpinePts(IDeA::numberOfRecords(*spineCurveData));
		for(size_t k(0); k < nbBaseSpinePts; ++k)
		{
			IDeA::inject(*spineCurveData, IDeA_KEY(SPINECURVETABLE, DF), k, 
						 exp(-IDeA::extract<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE,Y), k)));
		}

		//	Tenor Spread Surface Curves
		m_tenorSpreadSurface.fillWithIndexSpineCurvesData(spineCurvesData);

		//	Structure spine curve (its sets its own df values)
		spineCurvesData->set<LTQuant::GenericDataPtr>("Structure", 0, StructureSurfaceHolder::holdee().getCurveDetails());
		
		return spineCurvesDetailsData;
	}

    void MultiTenorOISModel::getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const
    {
        m_baseRateCurve->getUnfixedKnotPoints(points);

        //	Tenor Spread Surface Curves
        m_tenorSpreadSurface.getUnfixedIndexSpineCurvesKnotPoints(points);

        //	Structure spine curve 
        StructureSurfaceHolder::holdee().getUnfixedKnotPoints(points);
    }

    ostream& MultiTenorOISModel::print(ostream& out) const
    {
        // General settings
        const string baseRate( m_baseRate->getDescription());
        //const string discount( CurveType::Discount()->getDescription());
        out << "Base Rate:\t" << baseRate << endl;
        
        // ( should count the # of curves to adjust the # of "\t"
        out << "\t\t\t\t\t\t\t\t\t\tSpread Surface - Forward Integrals (bp)" << endl;
        
        // Column Headers (Maturity & curve Types)
        out << "Time to mat. (Y)\t" << baseRate;
        
        for(TenorSpreadSurface::const_iterator iter(m_tenorSpreadSurface.begin()); iter != m_tenorSpreadSurface.end(); ++iter)
        {
            out << "\t" << iter->first->getDescription();
        }

        // Headers for the discount and spread surface spread in basis points
        out << "\t";
    
        for(TenorSpreadSurface::const_iterator iter(m_tenorSpreadSurface.begin()); iter != m_tenorSpreadSurface.end(); ++iter)
        {
            out << "\t" << iter->first->getDescription();
        }

        out << endl;
        
        // Forward curves integrals
        typedef vector<double> TimesContainer;
        TimesContainer allTimes(m_tenorSpreadSurface.getFlowTimes());

        // Now all flow times are there, sort them
        sort(allTimes.begin(), allTimes.end());

        for(TimesContainer::const_iterator timeIter(allTimes.begin()); timeIter != allTimes.end(); ++timeIter)
        {
            out << (*timeIter) << "\t" << m_baseRateCurve->evaluate(*timeIter);
            
            for(TenorSpreadSurface::const_iterator curveIter(m_tenorSpreadSurface.begin()); curveIter != m_tenorSpreadSurface.end(); ++curveIter)
            {
                out << "\t" << curveIter->second->evaluate(*timeIter);
            }

            out << "\t";

            // Same results for discount and spread surface curves in basis points
            const double bpFactor(1.0e4);
            for(TenorSpreadSurface::const_iterator curveIter(m_tenorSpreadSurface.begin()); curveIter != m_tenorSpreadSurface.end(); ++curveIter)
            {
                out << "\t" << bpFactor * curveIter->second->evaluate(*timeIter);
            }
            
            out << endl;
        }
        return out;
    }

    /**
        @brief Create a clone.

        Uses a lookup table to ensure the directed graph relationships are maintained.

        @param lookup A lookup of previously created clones.
        @return       A clone of this instance.
    */
    ICloneLookupPtr MultiTenorOISModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new MultiTenorOISModel(*this, lookup));
    }
    
     /**
        @brief A pseudo copy constructor.

        Uses a lookup table to ensure the directed graph relationships are maintained.

        @param original The instance to be copied.
        @param lookup   A lookup of previously created clones.
    */
    MultiTenorOISModel::MultiTenorOISModel(MultiTenorOISModel const& original, CloneLookup& lookup) :
        MultiCurveModel(original, lookup),
        m_baseRate(original.m_baseRate),
        m_baseRateCurve(lookup.get(original.m_baseRateCurve)),
        m_tenorSpreadSurface(original.m_tenorSpreadSurface, lookup),
        m_numberOfBaseRateCurveUnknowns(original.m_numberOfBaseRateCurveUnknowns),
        m_totalNumberOfUnknowns(original.m_totalNumberOfUnknowns)
    {
    }

    void MultiTenorOISModel::throwError(const string& functionName, const string& errorMsg) const
    {
        LT_THROW_ERROR( "Error in function 'MultiTenorOISModel::" + functionName + "': " + errorMsg );
    }

    /// after calib is finished destory all of the calibration
    /// instruments so they do not take up memory via the CachedInstrument component
    /// the multiTenorOISModel has no intruments hence just forward upwards
    void MultiTenorOISModel::finishCalibration()
    {
        MultiCurveModel::finishCalibration();
    }

	void MultiTenorOISModel::getSpineInternalData(SpineDataCachePtr& sdp) const {
		m_baseRateCurve->getCurveInternalData(sdp->xy_);
		m_tenorSpreadSurface.getCurveInternalData(sdp->xy_);
	}

	void MultiTenorOISModel::assignSpineInternalData(SpineDataCachePtr& sdp) {
		knot_points_container::const_iterator p = sdp->xy_.begin();
		m_baseRateCurve->assignCurveInternalData(p++);
		m_tenorSpreadSurface.assignCurveInternalData(p++);
	}

}   //  FlexYCF