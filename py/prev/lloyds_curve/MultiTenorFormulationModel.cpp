
/*****************************************************************************

    FlexYCF model
    
    @Jarek Solowiej
    
    Copyright (C) Lloyds TSB Group plc 2012 All Rights Reserved

*****************************************************************************/





#include "stdafx.h"


//	FlexYCF
#include "MultiTenorFormulationModel.h"
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
#include "SpotRateCurve.h"
#include "CurveFormulationFactory.h"
#include "CurveFormulation.h"
#include "MinusLogDiscountCurve.h"
#include "UkpCurve.h"
#include "Maths\Problem.h"
#include "Data\GenericData.h"
#include "TblConversion.h"
//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"
#include "SpineDataCache.h"

using namespace std;

using namespace LTQC;

namespace FlexYCF
{    
    using namespace LTQuant;

    MultiTenorFormulationModel::MultiTenorFormulationModel(const LT::date valueDate, const LTQuant::GenericDataPtr& masterTable, const FlexYCFZeroCurvePtr parent) 
		: MultiCurveModel(*masterTable, parent), m_tenorSurface(*masterTable) 
	{
		JacobianIsNotSupported();
	}

    BaseModelPtr MultiTenorFormulationModel::createInstance(const LTQuant::GenericDataPtr masterTable, const FlexYCFZeroCurvePtr parent)
    {
        const GenericDataPtr modelParametersTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS)));
		const GenericDataPtr parametersTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, YC_CURVEPARAMETERS)));
    
		const GenericDataPtr detailsTable(IDeA::extract<LTQuant::GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE, CURVEDETAILS)));
        const LT::date valueDate(IDeA::extract<LT::date>(*detailsTable, IDeA_KEY(CURVEDETAILS, BUILDDATE)));
        
        MultiTenorFormulationModelPtr multiTenorModel(new MultiTenorFormulationModel(valueDate, masterTable, parent));

        LTQuant::GenericDataPtr curvesInterpolationTable;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(*modelParametersTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, CURVESINTERPOLATION), curvesInterpolationTable);
			
		if(curvesInterpolationTable)
		{ 
			LTQuant::GenericDataPtr interpolationDetailsTable;
			curvesInterpolationTable->permissive_get<LTQuant::GenericDataPtr>(CurveType::Discount()->getDescription(), 0, interpolationDetailsTable, LTQuant::GenericDataPtr());

			const string defaultCurveTypeName(UkpCurve::getName());
			string curveTypeName(defaultCurveTypeName);

			const string defaultCurveFormulationName(MinusLogDiscountCurve::getName());
			string curveFormulationName(defaultCurveFormulationName);
			const GenericDataPtr modelParametersTable(IDeA::extract<GenericDataPtr>(*masterTable, IDeA_KEY(YIELDCURVE,FLEXYC_MODELPARAMETERS)));
			
			if(curvesInterpolationTable->doesTagExist(CurveType::Discount()->getDescription()))
			{
				LTQuant::GenericDataPtr tenorCurveTable(curvesInterpolationTable->get<LTQuant::GenericDataPtr>(CurveType::Discount()->getDescription(), 0));
				tenorCurveTable->permissive_get<string>("Curve Type", 0, curveTypeName, defaultCurveTypeName);
				tenorCurveTable->permissive_get<string>("Formulation", 0, curveFormulationName, defaultCurveFormulationName);
			}	
			multiTenorModel->m_discountCurve = CurveFormulationFactory::createInstance(curveFormulationName, curvesInterpolationTable, CurveType::Discount()->getDescription(), multiTenorModel->getLeastSquaresResiduals());
			multiTenorModel->m_tenorSurface.createTenorCurves(curvesInterpolationTable, multiTenorModel->getLeastSquaresResiduals());
        }
        else
        {
            LT_THROW_ERROR("No 'Curves Interpolation' table provided to build the MultiTenorFormulationModel.");
        }
        return multiTenorModel;
    }

    CurveTypeConstPtr MultiTenorFormulationModel::getBaseRate() const
    {
        return CurveType::Discount();
    }

    double MultiTenorFormulationModel::getDiscountFactor(const double flowTime) const
    {
        return m_discountCurve->getDiscountFactor(flowTime) * exp( -StructureSurfaceHolder::holdee().getLogFvf(flowTime) );        

    }

    double MultiTenorFormulationModel::getTenorDiscountFactor(const double flowTime, const double tenor) const
    {
		return m_tenorSurface.interpolateCurve(tenor, flowTime) * exp(- StructureSurfaceHolder::holdee().getLogFvf(flowTime) );
    }

  
    void MultiTenorFormulationModel::accumulateDiscountFactorGradient(const double flowTime, double multiplier,  GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {       
        GradientIterator gradIter = gradientBegin;         
        m_discountCurve->accumulateDiscountFactorGradient(flowTime, multiplier * exp( - StructureSurfaceHolder::holdee().getLogFvf(flowTime) ), gradIter, gradIter + m_discountCurve->getNumberOfUnknowns());
    }

    void MultiTenorFormulationModel::accumulateTenorDiscountFactorGradient(const double flowTime, const double tenor, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        GradientIterator gradIter = gradientBegin;         
        gradIter += m_discountCurve->getNumberOfUnknowns();  
        m_tenorSurface.accumulateGradient(tenor, flowTime, multiplier * exp(- StructureSurfaceHolder::holdee().getLogFvf(flowTime)), gradIter, gradIter + m_tenorSurface.getNumberOfUnknowns());
    }

	void MultiTenorFormulationModel::accumulateDiscountFactorGradient(const double flowTime, 
														   double multiplier,
														   GradientIterator gradientBegin,
														   GradientIterator gradientEnd,
														   const CurveTypeConstPtr& curveType) const
	{
		m_discountCurve->accumulateDiscountFactorGradient(flowTime, multiplier *  exp( -StructureSurfaceHolder::holdee().getLogFvf(flowTime) ), gradientBegin, gradientEnd);
	}

	void MultiTenorFormulationModel::accumulateTenorDiscountFactorGradient(const double flowTime,
															    const double tenor, 
															    double multiplier, 
															    GradientIterator gradientBegin, 
															    GradientIterator gradientEnd,
															    const CurveTypeConstPtr& curveType) const
	{
		//m_tenorSurface.accumulateCurveGradient(CurveType::getFromYearFraction(tenor), flowTime, multiplier * exp(- StructureCurveHolder::holdee().getLogFvf(flowTime)), gradientBegin, gradientEnd, curveType);
	}

    void MultiTenorFormulationModel::onKnotPointsPlaced()
    {	
		const BaseCurvePtr baseRateCurveAsBaseCurve(std::tr1::dynamic_pointer_cast<BaseCurve>(m_discountCurve));
		if(baseRateCurveAsBaseCurve)
		{
			setSeparationPoint(getValueDate(), *baseRateCurveAsBaseCurve);
		}
	
        m_numberOfBaseRateCurveUnknowns = m_discountCurve->getNumberOfUnknowns();
        m_totalNumberOfUnknowns = m_numberOfBaseRateCurveUnknowns + m_tenorSurface.getNumberOfUnknowns();

    }

    void MultiTenorFormulationModel::addKnotPoint(const CurveTypeConstPtr curveType, const KnotPoint& knotPoint)
    {
        if(curveType == CurveType::Discount())
        {
            m_discountCurve->addKnotPoint(knotPoint);
        }
        else 
        {
            if(!m_tenorSurface.curveExists(curveType))
            {
				LT_THROW_ERROR( std::string("No curve of type '").append(curveType->getDescription()).append(" exists in the tenor spread surface") );
            }
            m_tenorSurface.addKnotPoint(curveType, knotPoint);
        }
    }

    void MultiTenorFormulationModel::finishCalibration()
    {
        BaseModel::finishCalibration();
		for_each(m_discountCurve->begin(),m_discountCurve->end(),[&](const KnotPoint& kp)
        {if(kp.getRelatedInstrument()) kp.getRelatedInstrument()->finishCalibration(BaseModelPtr(this,NullDeleter()));});

        for_each(m_tenorSurface.begin(),m_tenorSurface.end(),
            [&](const TenorSurface::const_iterator::value_type& curve){ 
                for_each(curve.second->begin(),curve.second->end(),[&](const KnotPoint& kp){
                    if(kp.getRelatedInstrument()) kp.getRelatedInstrument()->finishCalibration(BaseModelPtr(this,NullDeleter()));});});
        
    }

    void MultiTenorFormulationModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem)
    {
        
        m_discountCurve->addUnknownsToProblem(problem);
        m_tenorSurface.addUnknownsToProblem(problem);
    }

	void MultiTenorFormulationModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
									   			IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		m_discountCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
		m_tenorSurface.addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem, CurveType::AllTenors());
	}

	void MultiTenorFormulationModel::addVariablesToProblem(const LTQuant::ProblemPtr& problem,
												const CurveTypeConstPtr& curveType,
												IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		if(curveType == CurveType::Discount())
		{
			m_discountCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
		}
		else
		{
			m_tenorSurface.addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem, curveType);
		}
	}

	void MultiTenorFormulationModel::updateVariablesFromShifts(const LTQC::VectorDouble& variablesShifts)
	{
		
        size_t numberOfUnknownsDiscountSpreadCurve = m_discountCurve->getNumberOfUnknowns();
		LTQC::VectorDouble discountSpreadVariablesShifts(numberOfUnknownsDiscountSpreadCurve,0.0);
		for(size_t i = 0; i < numberOfUnknownsDiscountSpreadCurve; ++i)
		{
			discountSpreadVariablesShifts[i]  = variablesShifts[i];
		}
		m_discountCurve->updateVariablesFromShifts(discountSpreadVariablesShifts);
		m_tenorSurface.updateVariablesFromShifts(variablesShifts.begin() + numberOfUnknownsDiscountSpreadCurve, variablesShifts.end());
        
        update();
	};

    void MultiTenorFormulationModel::update()
    {
		m_discountCurve->update();
        m_tenorSurface.update();
    }

    void MultiTenorFormulationModel::finalize()
    {
		m_discountCurve->finalize();
        m_tenorSurface.finalize();    
    }

	void MultiTenorFormulationModel::initializeKnotPoints()
	{
		initializeKnotPoints(CurveType::Discount(), 0.0);
		initializeKnotPoints(CurveType::AllTenors(), 0.0);
	}

	void MultiTenorFormulationModel::initializeKnotPoints(const CurveTypeConstPtr& curveType,
			 								   const double initialSpotRate)
	{		
		if (curveType == CurveType::Discount())
		{
			m_discountCurve->initializeKnotPoints();
		}
		else
		{
			m_tenorSurface.initializeKnotPoints(curveType);
		}
	}

	double MultiTenorFormulationModel::getVariableValueFromSpineDiscountFactor(const double flowTime,
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

	size_t MultiTenorFormulationModel::getNumberOfUnknowns(const CurveTypeConstPtr& curveType) const
	{
		if(curveType == CurveType::Discount())
		{
			return m_discountCurve->getNumberOfUnknowns();
		}		
		else
		{
			return m_tenorSurface.getNumberOfUnknowns(curveType);
		}
	}

	LTQuant::GenericDataPtr MultiTenorFormulationModel::getSpineCurvesDetails() const
	{
		const LTQuant::GenericDataPtr spineCurvesDetailsData(new LTQuant::GenericData("Spine Curves Details", 0));
		spineCurvesDetailsData->set<std::string>("Base Rate", 0, "None");
		
		LTQuant::GenericDataPtr spineCurvesData(new LTQuant::GenericData("Spine Curves", 0));
		spineCurvesDetailsData->set<LTQuant::GenericDataPtr>("Curves", 0, spineCurvesData);

		LTQuant::GenericDataPtr spineCurveData;

		
		//	Funding Spine Curve
		spineCurveData = m_discountCurve->getSpineCurveDetails();
		spineCurvesData->set<LTQuant::GenericDataPtr>("Funding", 0, spineCurveData);
		const size_t nbFundingSpinePts(IDeA::numberOfRecords(*spineCurveData));
		for(size_t k(0); k < nbFundingSpinePts; ++k)
		{
			double df = IDeA::extract<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE,DF),k);
			IDeA::inject(*spineCurveData, IDeA_KEY(SPINECURVETABLE, DF), k, df);		
		}

		//	Tenor Spread Surface Curves
		m_tenorSurface.fillWithIndexSpineCurvesData(spineCurvesData);

		//	Structure spine curve (its sets its own df values)
		spineCurvesData->set<LTQuant::GenericDataPtr>("Structure", 0, StructureSurfaceHolder::holdee().getCurveDetails());
		
		return spineCurvesDetailsData;
	}

	void MultiTenorFormulationModel::getSpineInternalData(SpineDataCachePtr& sdp) const {
		m_discountCurve->getCurveInternalData(sdp->xy_);
		m_tenorSurface.getCurveInternalData(sdp->xy_);
	}

	void MultiTenorFormulationModel::assignSpineInternalData(SpineDataCachePtr& sdp) {
		knot_points_container::const_iterator p = sdp->xy_.begin();
		m_discountCurve->assignCurveInternalData(p++);
		m_tenorSurface.assignCurveInternalData(p++);
	}

    void MultiTenorFormulationModel::getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const
    {
        m_discountCurve->getSpineCurvesUnfixedKnotPoints(points);
        //	Tenor Spread Surface Curves
        m_tenorSurface.getIndexSpineCurvesUnfixedKnotPoints(points);
        //	Structure spine curve
        StructureSurfaceHolder::holdee().getUnfixedKnotPoints(points);
    }
   
    ICloneLookupPtr MultiTenorFormulationModel::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new MultiTenorFormulationModel(*this, lookup));
    }
    
    MultiTenorFormulationModel::MultiTenorFormulationModel(MultiTenorFormulationModel const& original, CloneLookup& lookup) :
        MultiCurveModel(original, lookup),
        m_discountCurve(lookup.get(original.m_discountCurve)),
		m_tenorSurface(original.m_tenorSurface, lookup),
        m_numberOfBaseRateCurveUnknowns(original.m_numberOfBaseRateCurveUnknowns),
        m_totalNumberOfUnknowns(original.m_totalNumberOfUnknowns)
    {
    }

    void MultiTenorFormulationModel::throwError(const string& functionName, const string& errorMsg) const
    {
        LT_THROW_ERROR( "Error in function 'MultiTenorFormulationModel::" + functionName + "': " + errorMsg );
    }
	
	const LTQC::Matrix& MultiTenorFormulationModel::getFullJacobian() const
    {
		if( !getDependentMarketData() )
        {
            return getJacobian();
        }
		BaseModelConstPtr   dependentModel;
        findDependentModel(dependentModel);
		const LTQC::Matrix&	dependentJacobian = dependentModel->getFullJacobian();
        const LTQC::Matrix&	jacobian = getJacobian();
        const size_t rows = dependentJacobian.getNumRows() + jacobian.getNumRows();
        const size_t cols = dependentJacobian.getNumCols() + jacobian.getNumCols();
        const size_t depRows = dependentJacobian.getNumRows();
        const size_t depCols = dependentJacobian.getNumCols();

        m_fullJacobian.resize(rows, cols);

        for(size_t row = 0; row < rows; ++row)
        {
                for(size_t col = 0; col < cols; ++col)
                {
                    if(row < depRows && col < depCols)
                    {
                    m_fullJacobian(row,col) = dependentJacobian(row,col);
                    }
                    else if(row >= depRows && col >= depCols)
                    {
                        m_fullJacobian(row,col) = jacobian(row - depRows, col - depCols);
                    } 
                    else
                    {
						m_fullJacobian(row,col) = 0.0;
                    }
                }
        }

		// x-terms
		CalibrationInstruments instruments = getFullInstruments();
        for(size_t row = depRows, rowJac = depRows; rowJac < rows; ++row)
        {
			if( instruments[row - depRows]->wasPlaced() )
			{
				vector<double> gradient( depCols, 0.0);
				instruments[row - depRows]->accumulateGradientDependentModel(*dependentModel, *this,1.0, gradient.begin(), gradient.end());

				for(size_t col = 0; col < depCols; ++col)
				{
					m_fullJacobian(rowJac,col) = gradient[col];
				}
				++rowJac;
			}
        }
		return m_fullJacobian;
	}
	
	LT::TablePtr MultiTenorFormulationModel::getFullJacobian( const bool includeHeadings ) const
    {
        return getJacobian(includeHeadings);
    }
    
	void MultiTenorFormulationModel::findDependentModel(BaseModelConstPtr&   dependentModel) const
    {
        LT::Str market; 
        if( !getDependentMarketData() )
        {
            LTQC_THROW( IDeA::ModelException, "Can not find any dependencies");
        }
		size_t numberIRCurves = 0;
        for (size_t i = 1; i < getDependentMarketData()->table->rowsGet(); ++i) 
        {
            IDeA::AssetDomainType adType = IDeA::AssetDomainType(IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, TYPE), i-1)); 
            LT::Str asset = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, ASSET), i-1); 
            
            if (adType == IDeA::AssetDomainType::IR )
            {
				++numberIRCurves;
                market = IDeA::extract<LT::Str>(getDependentMarketData()->table, IDeA_KEY(YC_DEPENDENTMARKETDATA, MARKET), i-1);
				if(hasDependentModel(IDeA::IRAssetDomain(asset, market)))
				{
					dependentModel = getDependentModel(IDeA::IRAssetDomain(asset, market));
					break;
				}
            }
        }
		if( numberIRCurves > 1)
        {
            LTQC_THROW( IDeA::ModelException, "Maximum of 1 dependent curves allowed");
        }   
    }

}   //  FlexYCF