#include "stdafx.h" 

//	FlexYCF
#include "TenorSurface.h"
#include "Interpolation.h"
#include "ICurveFactory.h"
#include "ICurve.h"
#include "LeastSquaresResiduals.h"
#include "KnotPointFunctor.h"
#include "TenorSurfaceInterpolation.h"
#include "FlexYCFCloneLookup.h"
#include "CurveUtils.h"
#include "CurveFormulationFactory.h"
#include "CurveFormulation.h"
#include "MinusLogDiscountCurve.h"
#include "UkpCurve.h"
#include "Maths\Problem.h"
#include "TblConversion.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"

#include "Data\GenericData.h"

using namespace std;



using namespace LTQC;

namespace FlexYCF
{
	TenorSurface::TenorSurface(const LTQuant::GenericData& masterTable)
	{
		m_tsInterpolation = TenorSurfaceInterpolation::createTenorSurfaceInterpolation(m_curves);
	}

   
    TenorSurface::TenorSurface(TenorSurface const& original, CloneLookup& lookup) :
        m_totalNumberOfUnknownKnotPoints(original.m_totalNumberOfUnknownKnotPoints),
        m_flowTimes(original.m_flowTimes)
    {
        for (TypedCurves::const_iterator it = original.m_curves.begin(); it != original.m_curves.end(); ++it)
        {
            m_curves.insert((*it).first, lookup.get((*it).second));
        }
       
        if (original.m_tsInterpolation.get() != 0)
		{
            m_tsInterpolation = original.m_tsInterpolation->clone(m_curves, lookup);
		}
    }

    TenorSurface::const_iterator TenorSurface::begin() const
    {
        return m_curves.begin();
    }

    TenorSurface::const_iterator TenorSurface::end() const
    {
        return m_curves.end();
    }

    vector<double> TenorSurface::getFlowTimes() const
    {
        return m_flowTimes;
    }

    bool TenorSurface::curveExists(const CurveTypeConstPtr& curveType) const
    {
        return m_curves.exists(curveType);
    }

    double TenorSurface::interpolateCurve(const double tenor, const double flowTime) const
    {
		return m_tsInterpolation->interpolate(tenor, flowTime);
	}

	/*void TenorSurface::accumulateCurveGradient(const CurveTypeConstPtr tenor,  const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, const CurveTypeConstPtr& curveType) const
    {
		m_tsInterpolation->accumulateGradient(tenor->getYearFraction(), flowTime, multiplier, gradientBegin, gradientEnd, curveType);
    }
    
    void TenorSurface::accumulateCurveGradient(const double tenor,  const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd, const CurveTypeConstPtr& curveType) const
    {
		m_tsInterpolation->accumulateGradient(tenor, flowTime, multiplier, gradientBegin, gradientEnd, curveType);
    }*/
    
    void TenorSurface::accumulateGradient(const double tenor, const double flowTime, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
		m_tsInterpolation->accumulateGradient(tenor, flowTime, multiplier, gradientBegin, gradientEnd);
    }

    size_t TenorSurface::getNumberOfUnknowns() const
    {
        size_t ukpAcc(0);
        for(TypedCurves::const_iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
        {
            ukpAcc += iter->second->getNumberOfUnknowns();
        }
        return ukpAcc;
    }

	size_t TenorSurface::getNumberOfUnknowns(const CurveTypeConstPtr& tenor) const
	{
		if(tenor == CurveType::AllTenors())
		{
			return getNumberOfUnknowns();
		}
		else if(!curveExists(tenor))
		{	
			return 0;
		}
		else
		{
			TypedCurves::const_iterator lower(m_curves.lowerBound(tenor));
			return(lower == m_curves.end() || lower->first !=  tenor ? 0 : lower->second->getNumberOfUnknowns());
		}
	}


    void TenorSurface::addKnotPoint(const CurveTypeConstPtr tenor, const KnotPoint& knotPoint)
    {
        checkIsTenor(tenor, "addKnotPoint");
       
        TypedCurves::const_iterator lower(m_curves.lowerBound(tenor));
        if(lower == m_curves.end() || lower->first != tenor)
        {
            // should never go through here now
            LT_THROW_ERROR("Attempt to add a knot-point a to tenor spread curve that has not be created.");
        }
        else
        {
            // otherwise just add the knot-point
            lower->second->addKnotPoint(knotPoint);
        }

        // Add the knot-point's time (x) to the set of flow-times if necessary
        vector<double>::iterator lower0(lower_bound(m_flowTimes.begin(), m_flowTimes.end(), knotPoint.x));
        if(lower0 == m_flowTimes.end() || (knotPoint.x < *lower0))
        {
            m_flowTimes.insert(lower0, knotPoint.x);
        }
	}

    void TenorSurface::update()
    {
        m_totalNumberOfUnknownKnotPoints = 0;
        for(TypedCurves::iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
        {
            iter->second->update();
            m_totalNumberOfUnknownKnotPoints += iter->second->getNumberOfUnknowns();   
        }
    }

    void TenorSurface::finalize()
    {
        for(TypedCurves::iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
        {
			(*iter).second->finalize();
        }
		m_tsInterpolation->finalize();
    }

    void TenorSurface::addUnknownsToProblem(const LTQuant::ProblemPtr& problem)
    {
        for(TypedCurves::iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
        {
            iter->second->addUnknownsToProblem(problem);
        }
    }
	
	void TenorSurface::addUnknownsToProblem(const LTQuant::ProblemPtr& problem, IKnotPointFunctor& onKnotPointVariableAddedToProblem, const CurveTypeConstPtr& curveType)
	{
		if(curveType == CurveType::AllTenors())
		{
			// Add all the unknowns on the tenor spread surface
			for(TypedCurves::iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
			{
				iter->second->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
			}
		} 
		else
		{
			// Add only the unknowns on the specified curve type
			TypedCurves::const_iterator iter(m_curves.lowerBound(curveType));
			if(iter != m_curves.end() && curveType == iter->first)
			{
				iter->second->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
			}
		}
	}

	void TenorSurface::updateVariablesFromShifts(const LTQC::VectorDouble::const_iterator shiftsBegin, const LTQC::VectorDouble::const_iterator shiftsEnd)
	{
		size_t nbVariablesSoFar, tmp(0);
		for(TypedCurves::iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
		{
			nbVariablesSoFar = tmp;
			tmp += iter->second->getNumberOfUnknowns();
			LTQC::VectorDouble shifts(shiftsBegin + nbVariablesSoFar, shiftsBegin + tmp);
			iter->second->updateVariablesFromShifts(shifts);
		}
	}

    

	void TenorSurface::initializeKnotPoints(const CurveTypeConstPtr& tenor)
	{
		if(tenor == CurveType::AllTenors())
		{
			for(TypedCurves::const_iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
			{
				iter->second->initializeKnotPoints();
			}
		}
		else
		{
			const TypedCurves::const_iterator iter(m_curves.lowerBound(tenor));
			if(iter != m_curves.end() && iter->first == tenor)
			{
				iter->second->initializeKnotPoints();
			}
		}
	}

	void TenorSurface::fillWithIndexSpineCurvesData(const LTQuant::GenericDataPtr& spineCurvesData) const
	{
		LTQuant::GenericDataPtr spineCurveData;

		for(TypedCurves::const_iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
		{
			spineCurveData = iter->second->getSpineCurveDetails();	
			spineCurvesData->set<LTQuant::GenericDataPtr>(iter->first->getDescription(), 0, spineCurveData);
			const size_t nbTenorBasisSpinePts(IDeA::numberOfRecords(*spineCurveData));
			for(size_t k(0); k < nbTenorBasisSpinePts; ++k)
			{	
				double df = IDeA::extract<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE,DF),k);
				IDeA::inject(*spineCurveData, IDeA_KEY(SPINECURVETABLE, DF), k, df);		
			}
		}
	}

	void TenorSurface::getCurveInternalData(knot_points_container& kpc) const {
		for (auto p = begin(); p != end(); ++p)
			p->second->getCurveInternalData(kpc);
	}

	void TenorSurface::assignCurveInternalData(knot_points_container::const_iterator it) {
		for (auto p = begin(); p != end(); ++p)
			p->second->assignCurveInternalData(it++);
	}

    void TenorSurface::getIndexSpineCurvesUnfixedKnotPoints(std::list<double>& points) const
    {
        for(TypedCurves::const_iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
        {
            iter->second->getSpineCurvesUnfixedKnotPoints(points);	
        }
    }

    void TenorSurface::createTenorCurves(const LTQuant::GenericDataPtr curvesInterpolationTable, const LeastSquaresResidualsPtr leastSquaresResiduals)
    {

        for(CurveType::Tenors::const_iterator tenorIter(CurveType::Tenors::begin());tenorIter != CurveType::Tenors::end();++tenorIter)
        {
		   if((*tenorIter)->getDescription() != CurveType::Discount()->getDescription())
		   {
				LTQuant::GenericDataPtr interpolationDetailsTable;
				curvesInterpolationTable->permissive_get<LTQuant::GenericDataPtr>((*tenorIter)->getDescription(), 0, interpolationDetailsTable, LTQuant::GenericDataPtr());
				const string defaultCurveTypeName(UkpCurve::getName());
				string curveTypeName(defaultCurveTypeName);

				const string defaultCurveFormulationName(MinusLogDiscountCurve::getName());
				string curveFormulationName(defaultCurveFormulationName);
			
				if(interpolationDetailsTable)
				{
					interpolationDetailsTable->permissive_get<string>("Curve Type", 0, curveTypeName, defaultCurveTypeName);
					interpolationDetailsTable->permissive_get<string>("Formulation", 0, curveFormulationName, defaultCurveFormulationName);
					CurveFormulationPtr curve = CurveFormulationFactory::createInstance(curveFormulationName, curvesInterpolationTable, (*tenorIter)->getDescription(), leastSquaresResiduals);
					m_curves.insert(*tenorIter, curve);
				}
		   }
        }
	}

    /// Returns the number of unknowns on the curves whose tenor is stricly less than the specified tenor
    size_t TenorSurface::getNumberOfUnknownsUpToTenor(const double tenor) const
    {
        size_t numberOfUnknowns(0);
        for(TypedCurves::const_iterator iter(m_curves.begin()); iter != m_curves.end(); ++iter)
        {
            if(iter->first->getYearFraction() < tenor)
            {
                numberOfUnknowns += iter->second->getNumberOfUnknowns();
            }
        }
        return numberOfUnknowns;
    }


    void TenorSurface::checkIsTenor(const CurveTypeConstPtr curveType, const std::string& checkedFunctionName) const
    {
        if(!curveType->isTenor())
        {
            LT_THROW_ERROR("Error in function TenorSpreadSurface::" + checkedFunctionName  + ": the curve type '" + curveType->getDescription() + "' specified is not a tenor.");
        }
    }

    bool TenorSurface::YearFractionKeyValueCompare::operator()(const KeyValuePair& lhs, const KeyValuePair& rhs) const
    {
        return operator()(lhs.first->getYearFraction(), rhs.first->getYearFraction());
    }
    
    bool TenorSurface::YearFractionKeyValueCompare::operator()(const KeyValuePair& lhs, const double rhs) const
    {
        return operator()(lhs.first->getYearFraction(), rhs);
    }

    bool TenorSurface::YearFractionKeyValueCompare::operator()(const double lhs, const KeyValuePair& rhs) const
    {
        return operator()(lhs, rhs.first->getYearFraction());
    }

    bool TenorSurface::YearFractionKeyValueCompare::operator()(const double lhs, const double rhs) const
    {
        return lhs < rhs;
    }

}