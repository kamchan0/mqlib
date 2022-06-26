#include "stdafx.h"


//	FlexYCF
#include "MultiTenorOISFundingModel.h"
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

    MultiTenorOISFundingModel::MultiTenorOISFundingModel(const LT::date valueDate,
                                     const CurveTypeConstPtr& baseRate,
									 const LTQuant::GenericDataPtr& masterTable,
                                     const FlexYCFZeroCurvePtr parent):
        MultiCurveModel(*masterTable, parent),
		m_baseRate(baseRate),
        m_overnightRate(CurveType::ON()),
        m_overnightTenor(m_overnightRate->getYearFraction()),
        m_tenorSpreadSurface(baseRate, *masterTable)
    {
    }

    BaseModelPtr MultiTenorOISFundingModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
        const GenericDataPtr modelParametersTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS)));

        LT_LOG << "Creating MultiTenorOISFunding Model" << std::endl;
         // The following is for DEBUG only
		const GenericDataPtr parametersTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
        string currency("");
		IDeA::permissive_extract<std::string>(*parametersTable, IDeA_KEY(YC_CURVEPARAMETERS, CURRENCY), currency);

		LT_LOG << "currency: " << currency << std::endl;

        // Retrieve the value date
		const GenericDataPtr detailsTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        // Retrieve the base rate
        string baseRateDescription(CurveType::ON()->getDescription());
		IDeA::permissive_extract<std::string>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, BASERATE), baseRateDescription, CurveType::_3M()->getDescription());

        const CurveTypeConstPtr baseRate(CurveType::getFromDescription(baseRateDescription));
        if( baseRate == CurveType::ON() || baseRate == CurveType::Discount() )
        {
            LT_THROW_ERROR("Base rate can not be ON nor Discount in MultiTenorOISFundingModel.");
        }

        // Create the MultiTenorOISFundingModel
        MultiTenorOISFundingModelPtr multiTenorModel(new MultiTenorOISFundingModel(valueDate, baseRate, masterTable, parent));

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

            // If the base rate curve is not the discount curve, create the discount spread curve
            if(baseRate != CurveType::Discount())
            {
                curvesInterpolationTable->permissive_get<LTQuant::GenericDataPtr>(CurveType::Discount()->getDescription(), 0, interpolationDetailsTable, LTQuant::GenericDataPtr());
                multiTenorModel->m_discountSpreadCurve = ICurveFactory::createInstance(interpolationDetailsTable, multiTenorModel->getLeastSquaresResiduals());
            }

            // Create the tenor spread surface curves
            multiTenorModel->m_tenorSpreadSurface.createTenorSpreadCurves(curvesInterpolationTable, multiTenorModel->getLeastSquaresResiduals());
        }
        else
        {
            LT_THROW_ERROR("No 'Curves Interpolation' table provided to build the MultiTenorOISFundingModel.");
        }

        return multiTenorModel;
    }

    /// after calib is finished destory all of the calibration
    /// instruments so they do not take up memory via the CachedInstrument component
    /// however this model has no instruments of its own
    void MultiTenorOISFundingModel::finishCalibration()
    {
        MultiCurveModel::finishCalibration();
    }
    CurveTypeConstPtr MultiTenorOISFundingModel::getBaseRate() const
    {
        return m_baseRate;
    }

    double MultiTenorOISFundingModel::getDiscountFactor(const double flowTime) const
    {
        return exp( -( m_baseRateCurve->evaluate(flowTime)  + m_tenorSpreadSurface.interpolateCurve( m_overnightTenor, flowTime)  + m_discountSpreadCurve->evaluate(flowTime) + StructureSurfaceHolder::holdee().getLogFvf(flowTime,m_overnightRate)));        
    }

    double MultiTenorOISFundingModel::getTenorDiscountFactor(const double flowTime, const double tenor) const
    {
        return exp( -( m_baseRateCurve->evaluate(flowTime)  + m_tenorSpreadSurface.interpolateCurve(tenor, flowTime) + StructureSurfaceHolder::holdee().getLogFvf(flowTime,tenor) ));
    }

    // For a point Ri on the base rate curve, the partial derivative dP(t) / dRi can we decomposed as:
    //  dP(t) / dR(t)  x  dR(t) / dRi.  From there, dP(t)/dR(t) = - P(t)  because P(t) = exp{-[R(t) + DiscSpread(t)]}, and can be computed
    //  directly from the model. dR(t) / dRi is computed by a virtual function in the curve class that delegates calculation
    //  of gradient to its interpolation method class.
    //  Note:
    // in the multi-tenors model, the discount factor P(t) is only sensitive to 8 unknowns at most (depending on the interpolation method):
    // the two knot points immediately on the left of t and the two knot points immediately on the right of the base rate curve and the discounted 
    // spread curve.
    void MultiTenorOISFundingModel::accumulateDiscountFactorGradient(const double flowTime,
                                                           double multiplier,
                                                           GradientIterator gradientBegin,
                                                           GradientIterator gradientEnd) const
    {       
        multiplier *= -getDiscountFactor(flowTime);
      
        GradientIterator gradIter = gradientBegin;         
        m_baseRateCurve->accumulateGradient(flowTime, multiplier, gradIter, gradIter + m_baseRateCurve->getNumberOfUnknowns());

        gradIter += m_baseRateCurve->getNumberOfUnknowns();
        m_discountSpreadCurve->accumulateGradient(flowTime, multiplier, gradIter, gradIter + m_discountSpreadCurve->getNumberOfUnknowns());
        
        if(m_baseRate != m_overnightRate)
        {
            gradIter += m_discountSpreadCurve->getNumberOfUnknowns();  
            m_tenorSpreadSurface.accumulateCurveGradient(m_overnightTenor, flowTime, multiplier, gradIter, gradIter + m_tenorSpreadSurface.getNumberOfUnknowns());
        }
    }

    void MultiTenorOISFundingModel::accumulateTenorDiscountFactorGradient(const double flowTime, 
                                                                const double tenor,
                                                                double multiplier,
                                                                GradientIterator gradientBegin,
                                                                GradientIterator gradientEnd) const
    {
        multiplier *= -getTenorDiscountFactor(flowTime, tenor);
    
        GradientIterator gradIter = gradientBegin;
        m_baseRateCurve->accumulateGradient(flowTime, multiplier, gradIter, gradIter + m_baseRateCurve->getNumberOfUnknowns());

        gradIter += m_baseRateCurve->getNumberOfUnknowns();  
        gradIter += m_discountSpreadCurve->getNumberOfUnknowns();   // should work even when base == discount

        m_tenorSpreadSurface.accumulateCurveGradient(tenor, flowTime, multiplier, gradIter, gradIter + m_tenorSpreadSurface.getNumberOfUnknowns());
    }

	void MultiTenorOISFundingModel::accumulateDiscountFactorGradient(const double flowTime, 
														   double multiplier,
														   GradientIterator gradientBegin,
														   GradientIterator gradientEnd,
														   const CurveTypeConstPtr& curveType) const
	{
        multiplier *= -getDiscountFactor(flowTime);
    
		if(curveType == m_baseRate)
		{
			m_baseRateCurve->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
		}
		else if (curveType == CurveType::Discount())
		{
			m_discountSpreadCurve->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
		} 
        else if ( curveType == m_overnightRate )
        {
            m_tenorSpreadSurface.accumulateCurveGradient(m_overnightTenor, flowTime, multiplier, gradientBegin, gradientEnd);
        }
	}

	void MultiTenorOISFundingModel::accumulateTenorDiscountFactorGradient(const double flowTime,
															    const double tenor, 
															    double multiplier, 
															    GradientIterator gradientBegin, 
															    GradientIterator gradientEnd,
															    const CurveTypeConstPtr& curveType) const
	{
		
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

    void MultiTenorOISFundingModel::onKnotPointsPlaced()
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
        m_totalNumberOfUnknowns = m_numberOfBaseRateCurveUnknowns  + m_discountSpreadCurve->getNumberOfUnknowns() + m_tenorSpreadSurface.getNumberOfUnknowns();
    }

    void MultiTenorOISFundingModel::addKnotPoint(const CurveTypeConstPtr curveType, const KnotPoint& knotPoint)
    {
        // add the knot point to the right curve
        if(curveType == m_baseRate)
        {
            m_baseRateCurve->addKnotPoint(knotPoint);
        }
        else if(curveType == CurveType::Discount() || curveType == m_overnightRate)
        {
            // m_discountSpreadCurve->addKnotPoint(knotPoint);
            if(!m_tenorSpreadSurface.curveExists(m_overnightRate))
            {
				LT_THROW_ERROR( std::string("No curve of type '").append(m_overnightRate->getDescription()).append(" exists in the tenor spread surface") );
            }
            m_tenorSpreadSurface.addKnotPoint(m_overnightRate, knotPoint);
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

    void MultiTenorOISFundingModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem)
    {
        // 1. add the Unknown Knot-Points (UKP) of the base curve to the problem
        m_baseRateCurve->addUnknownsToProblem(problem);

		// 2. add the UKPs of the discount spread curve to the problem
        m_discountSpreadCurve->addUnknownsToProblem(problem);

        // 3. add the UKPs of the various tenor spread curves to the problem
        m_tenorSpreadSurface.addUnknownsToProblem(problem);
    }

	void MultiTenorOISFundingModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
									   			IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		m_baseRateCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem); 
		m_discountSpreadCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
		m_tenorSpreadSurface.addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem, CurveType::AllTenors());
	}

	void MultiTenorOISFundingModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
												const CurveTypeConstPtr& curveType,
												IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		if(curveType == m_baseRate)
		{
			m_baseRateCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
		} 
		else if(curveType == CurveType::Discount() || curveType == m_overnightRate)
		{
             m_tenorSpreadSurface.addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem, m_overnightRate);
			// m_discountSpreadCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
		}
		else
		{
			m_tenorSpreadSurface.addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem, curveType);
		}
	}

	void MultiTenorOISFundingModel::updateVariablesFromShifts(const LTQC::VectorDouble& variablesShifts)
	{
		//	TODO: check # of variable shifts = # of unknowns

		size_t k(0);

		//	Shift variables on the base spine curve
		size_t numberOfUnknowns(m_baseRateCurve->getNumberOfUnknowns());
		for( ; k < numberOfUnknowns; ++k)
		{
            if( variablesShifts[k] != 0.0 )
            {
			    m_baseRateCurve->shiftUnknown(k, variablesShifts[k]);
            }
		}
		
        //	Shift variables on the funding spine curve
        size_t numberOfUnknownsDiscountSpreadCurve = m_discountSpreadCurve->getNumberOfUnknowns();
		numberOfUnknowns += numberOfUnknownsDiscountSpreadCurve;
		for(size_t i = 0; i < numberOfUnknownsDiscountSpreadCurve; ++i,++k)
		{
            if( variablesShifts[k] != 0.0 )
            {
			    m_discountSpreadCurve->shiftUnknown(i, variablesShifts[k]);
            }
		}
		
        //	Shift variables on the tenor spread surface
		//	Note: must pass only the appropriate subset of the variables to shift 
		m_tenorSpreadSurface.updateVariablesFromShifts(variablesShifts.begin() + numberOfUnknowns, variablesShifts.end());
        
        update();
	};

    void MultiTenorOISFundingModel::update()
    {
        // Update the base rate curve
        m_baseRateCurve->update();

        m_discountSpreadCurve->update();
        
        // Update the tenor spread surface
        m_tenorSpreadSurface.update();
    }

    void MultiTenorOISFundingModel::finalize()
    {
		 // Finalize the base rate curve
		finalizeLogFvfCurve(*m_baseRateCurve);

		 // Finalize the discount spread curve
		finalizeLogFvfCurve(*m_discountSpreadCurve);
		
        // Finalize the tenor spread surface
        m_tenorSpreadSurface.finalize();    
    }

	void MultiTenorOISFundingModel::initializeKnotPoints()
	{
		// Default initialization to ease testing
		initializeKnotPoints(m_baseRate, 0.03);
		if(m_baseRate != CurveType::Discount())
		{
			initializeKnotPoints(CurveType::Discount(), 0.0);
		}
		initializeKnotPoints(CurveType::AllTenors(), 0.0);
	}

	void MultiTenorOISFundingModel::initializeKnotPoints(const CurveTypeConstPtr& curveType,
			 								   const double initialSpotRate)
	{
		LT_LOG << "Initializing curve " << curveType->getDescription() << " with spot rate " << initialSpotRate;

		// the function passed relies on logFvf representation
		if(curveType == m_baseRate)
		{
			m_baseRateCurve->initialize([initialSpotRate] (double x) {return multiTenorInitializationFunc(initialSpotRate,x);});
		}
		else if (curveType == CurveType::Discount())
		{
			m_discountSpreadCurve->initialize([initialSpotRate] (double x) {return multiTenorInitializationFunc(initialSpotRate,x);});
		}
		else
		{
			m_tenorSpreadSurface.initializeKnotPoints(curveType, initialSpotRate);
		}
	}

	double MultiTenorOISFundingModel::getVariableValueFromSpineDiscountFactor(const double flowTime,
																	const double discountFactor,
																	const CurveTypeConstPtr& /* curveType */) const
	{
		//  Note: specifying the curve type will allow the curves of the model
		//	to have different formulations. For now, all formulations are
		//	hardcoded as LogFvf so no use
		double variableValue(-log(discountFactor));

		LT_LOG << "getting var. value: " << variableValue << " from df: " << discountFactor << "@ time: " << flowTime << std::endl;
		
		return variableValue;
	}

	size_t MultiTenorOISFundingModel::getNumberOfUnknowns(const CurveTypeConstPtr& curveType) const
	{
		if(curveType == m_baseRate)
		{
			return m_baseRateCurve->getNumberOfUnknowns();
		}
		else if(curveType == CurveType::Discount())
		{
			return m_discountSpreadCurve->getNumberOfUnknowns();
		}		
		else
		{
			return m_tenorSpreadSurface.getNumberOfUnknowns(curveType);
		}
	}

	LTQuant::GenericDataPtr MultiTenorOISFundingModel::getSpineCurvesDetails() const
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

		//	Funding Spine Curve
		spineCurveData = m_discountSpreadCurve->getCurveDetails();
		spineCurvesData->set<LTQuant::GenericDataPtr>("Funding Spread", 0, spineCurveData);
		const size_t nbFundingSpinePts(IDeA::numberOfRecords(*spineCurveData));
		for(size_t k(0); k < nbFundingSpinePts; ++k)
		{
			IDeA::inject(*spineCurveData, IDeA_KEY(SPINECURVETABLE, DF), k, 
						 exp(-IDeA::extract<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE,Y), k)));		
		}

		//	Tenor Spread Surface Curves
		m_tenorSpreadSurface.fillWithIndexSpineCurvesData(spineCurvesData);
		
		//	Structure spine curve (its sets its own df values)
		const std::vector<CurveTypeConstPtr>& structeCurveTypes = StructureSurfaceHolder::holdee().curveTypes();
		if(structeCurveTypes.size() > 1)
		{
			for(size_t i=0; i < structeCurveTypes.size(); ++i)
			{
				std::string outputStr = "Structure " + structeCurveTypes[i]->getDescription();
				spineCurvesData->set<LTQuant::GenericDataPtr>(outputStr,0,StructureSurfaceHolder::holdee().getCurveDetails(structeCurveTypes[i]));
			}
		}
		else
		{
			spineCurvesData->set<LTQuant::GenericDataPtr>("Structure", 0, StructureSurfaceHolder::holdee().getCurveDetails());
		}

		return spineCurvesDetailsData;
	}

	 void MultiTenorOISFundingModel::getSpineInternalData(SpineDataCachePtr& sdp) const {
		m_baseRateCurve->getCurveInternalData(sdp->xy_);
		m_discountSpreadCurve->getCurveInternalData(sdp->xy_);
		m_tenorSpreadSurface.getCurveInternalData(sdp->xy_);
	}

	void MultiTenorOISFundingModel::assignSpineInternalData(SpineDataCachePtr& sdp) {
		auto p = sdp->xy_.begin();
		m_baseRateCurve->assignCurveInternalData(p++);
		m_discountSpreadCurve->assignCurveInternalData(p++);
		m_tenorSpreadSurface.assignCurveInternalData(p++);
	}

    void MultiTenorOISFundingModel::getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const
    {
        
        m_baseRateCurve->getUnfixedKnotPoints(points);
        m_discountSpreadCurve->getUnfixedKnotPoints(points);
        //	Tenor Spread Surface Curves
        m_tenorSpreadSurface.getUnfixedIndexSpineCurvesKnotPoints(points);

        //	Structure spine curve 
        const std::vector<CurveTypeConstPtr>& structureCurveTypes = StructureSurfaceHolder::holdee().curveTypes();
        if(structureCurveTypes.size() > 1)
        {
            for(size_t i=0; i < structureCurveTypes.size(); ++i)
            {
                StructureSurfaceHolder::holdee().getUnfixedKnotPoints(structureCurveTypes[i],points);
            }
        }
        else
        {
            StructureSurfaceHolder::holdee().getUnfixedKnotPoints(points);
        }
    }

    ostream& MultiTenorOISFundingModel::print(ostream& out) const
    {
        // General settings
        const string baseRate( m_baseRate->getDescription());
        const string discount( CurveType::Discount()->getDescription());
        out << "Base Rate:\t" << baseRate << endl;
        
        // ( should count the # of curves to adjust the # of "\t"
        out << "\t\t\t\t\t\t\t\t\t\tSpread Surface - Forward Integrals (bp)" << endl;
        
        // Column Headers (Maturity & curve Types)
        out << "Time to mat. (Y)\t" << baseRate;
        
        if(baseRate != discount)
            out << "\t" << discount;

        for(TenorSpreadSurface::const_iterator iter(m_tenorSpreadSurface.begin()); iter != m_tenorSpreadSurface.end(); ++iter)
        {
            out << "\t" << iter->first->getDescription();
        }

        // Headers for the discount and spread surface spread in basis points
        out << "\t";
        if(baseRate != discount)
            out << "\t" << discount;

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
            //const date MaturityDate(addyears(); out << MaturityDate  << "\t" 
            out << (*timeIter) << "\t" << m_baseRateCurve->evaluate(*timeIter);

            if(baseRate != discount)
                out << "\t" << m_discountSpreadCurve->evaluate(*timeIter);
            
            for(TenorSpreadSurface::const_iterator curveIter(m_tenorSpreadSurface.begin()); curveIter != m_tenorSpreadSurface.end(); ++curveIter)
            {
                out << "\t" << curveIter->second->evaluate(*timeIter);
            }

            out << "\t";

            // Same results for discount and spread surface curves in basis points
            const double bpFactor(1.0e4);

            if(baseRate != discount)
            {
                out << "\t" << bpFactor * m_discountSpreadCurve->evaluate(*timeIter);
            }

            for(TenorSpreadSurface::const_iterator curveIter(m_tenorSpreadSurface.begin()); curveIter != m_tenorSpreadSurface.end(); ++curveIter)
            {
                out << "\t" << bpFactor * curveIter->second->evaluate(*timeIter);
            }
            
            out << endl;
        }

        return out;
    }

   
    ICloneLookupPtr MultiTenorOISFundingModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new MultiTenorOISFundingModel(*this, lookup));
    }
    
  
    MultiTenorOISFundingModel::MultiTenorOISFundingModel(MultiTenorOISFundingModel const& original, CloneLookup& lookup) :
        MultiCurveModel(original, lookup),
        m_baseRate(original.m_baseRate),
        m_overnightRate(original.m_overnightRate),
        m_overnightTenor(original.m_overnightTenor),
        m_baseRateCurve(lookup.get(original.m_baseRateCurve)),
        m_discountSpreadCurve(lookup.get(original.m_discountSpreadCurve)),
        m_tenorSpreadSurface(original.m_tenorSpreadSurface, lookup),
        m_numberOfBaseRateCurveUnknowns(original.m_numberOfBaseRateCurveUnknowns),
        m_totalNumberOfUnknowns(original.m_totalNumberOfUnknowns)
    {
    }

    void MultiTenorOISFundingModel::throwError(const string& functionName, const string& errorMsg) const
    {
        LT_THROW_ERROR( "Error in function 'MultiTenorOISFundingModel::" + functionName + "': " + errorMsg );
    }
}   //  FlexYCF