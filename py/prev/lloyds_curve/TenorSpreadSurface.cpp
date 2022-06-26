/*****************************************************************************
    
	TenorSpreadSurface

	Implementation of the TenorSpreadSurface class


    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h" 

//	FlexYCF
#include "TenorSpreadSurface.h"
#include "Interpolation.h"
#include "ICurveFactory.h"
#include "ICurve.h"
#include "LeastSquaresResiduals.h"
#include "KnotPointFunctor.h"
#include "BaseTssInterpolation.h"
#include "FlexYCFCloneLookup.h"
#include "TssInterpolationFactory.h"
#include "BasicTssInterpolation.h"
#include "CurveUtils.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"

#include "Data\GenericData.h"

using namespace std;
using namespace LTQC;
using namespace LTQuant;

namespace
{
	std::string defaultTenorSpreadSurfaceInterpolationTypeName()
	{
		static const std::string defaultTenorSpreadSurfaceInterpolationTypeName_(FlexYCF::BasicTssInterpolation::getName());
		return defaultTenorSpreadSurfaceInterpolationTypeName_;
	}
}

namespace FlexYCF
{
	double multiTenorInitializationFunc(const double spotRate, const double x)
	{
		return (x < 0.0? 0.0: x * spotRate);
	}


    // TO DO: pass the correct table and least squares residuals
    TenorSpreadSurface::TenorSpreadSurface(const CurveTypeConstPtr& baseRate):
        m_baseRate(baseRate)
    { 
        // If the base rate is a tenor, create a zero spread curve over the base rate set
        //  with only the (0.0, 0.0) point and flat-right interpolation
        if(m_baseRate->isTenor())
        {   
            m_spreadCurves.insert(m_baseRate, ICurveFactory::createFlatCurve(0.0));
        }
		m_tssInterpolation = TssInterpolationFactory::createTssInterpolation(defaultTenorSpreadSurfaceInterpolationTypeName(), m_baseRate, m_spreadCurves, LTQuant::GenericDataPtr());
    }

	TenorSpreadSurface::TenorSpreadSurface(const CurveTypeConstPtr& baseRate,
										   const LTQuant::GenericData& masterTable):
		m_baseRate(baseRate)
	{
		if(m_baseRate->isTenor())
		{
			m_spreadCurves.insert(m_baseRate, ICurveFactory::createFlatCurve(0.0));
		}

		//	Create the Tenor Spread Surface Interpolation
		const std::string defaultTssInterpName(defaultTenorSpreadSurfaceInterpolationTypeName());
		std::string tssInterpName(defaultTssInterpName);
			
		LTQuant::GenericDataPtr tssInterpParamsTable;

		GenericDataPtr modelParamsTable;
		IDeA::permissive_extract<LTQuant::GenericDataPtr>(masterTable, IDeA_KEY(YIELDCURVE, FLEXYC_MODELPARAMETERS), modelParamsTable);

		if(modelParamsTable)
		{
			IDeA::permissive_extract<std::string>(*modelParamsTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, TSSI), tssInterpName, defaultTenorSpreadSurfaceInterpolationTypeName());
			IDeA::permissive_extract<LTQuant::GenericDataPtr>(*modelParamsTable, IDeA_KEY(FLEXYC_MODELPARAMETERS, TSSIPARAMETERS), tssInterpParamsTable);
		}

		m_tssInterpolation = TssInterpolationFactory::createTssInterpolation(tssInterpName, m_baseRate, m_spreadCurves, tssInterpParamsTable);
	}

    /**
        @brief Pseudo-copy constructor.

        Create a copy that preserves the directed graph relationships of the original.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    TenorSpreadSurface::TenorSpreadSurface(TenorSpreadSurface const& original, CloneLookup& lookup) :
        m_baseRate(original.m_baseRate),
        m_totalNumberOfUnknownKnotPoints(original.m_totalNumberOfUnknownKnotPoints),
        m_flowTimes(original.m_flowTimes),
        m_smallTenorCurveGradient(original.m_smallTenorCurveGradient),
        m_cachedCurveTypes(original.m_cachedCurveTypes)
    {
        // Copy across the spread curves cloning as we go
        for (TypedCurves::const_iterator it = original.m_spreadCurves.begin(); it != original.m_spreadCurves.end(); ++it)
        {
            // Share keys with original but have our own curves
            m_spreadCurves.insert((*it).first, lookup.get((*it).second));
        }
       
	
        // Clone the tenor spread surface interpolation. This (inadvisedly) uses a reference to our m_spreadCurves member. Have to ensure this
        // is correctly populated before cloning.
        if (original.m_tssInterpolation.get() != 0)
            m_tssInterpolation = original.m_tssInterpolation->clone(m_baseRate, m_spreadCurves, original.m_tssInterpolation->TSSInterpParameters(), lookup);
    }

    // TO REVIEW
	/*
    TenorSpreadSurface::TenorSpreadSurface(const CurveTypeConstPtr baseRate,
                                           const LTQuant::GenericDataPtr masterTable):
        m_baseRate(baseRate)
    {
        initCurveCacheInfo(masterTable);

        // If the base rate is a tenor, create a zero spread curve over the base rate set
        //  with only the (0.0, 0.0) point and flat-right interpolation
        if(m_baseRate->isTenor())
        {   
            // TypedCurve zeroSpreadCurve(baseRate, true);
            TypedCurvePtr zeroSpreadCurve(new TypedCurve(baseRate, true));

            zeroSpreadCurve = std::tr1::dynamic_pointer_cast<TypedCurve>(
                ICurveFactory::createInstance(LTQuant::GenericDataPtr(), LeastSquaresResidualsPtr()));
        }
    }

    void TenorSpreadSurface::initCurveCacheInfo(const LTQuant::GenericDataPtr masterTable)
    {
        const bool cacheTableExists(masterTable->doesTagExist("Cache Parameters") != NULL);
        GenericDataPtr cacheParametersTable;
        
        if(cacheTableExists)
        {
            cacheParametersTable = masterTable->get<LTQuant::GenericDataPtr>("Cache Parameters", 0);
        }

        vector<CurveTypeConstPtr> cachableTenorSpreadCurves; // we don't want to be able to cache ANY typed curve
        
        cachableTenorSpreadCurves.push_back(CurveType::ON());
        cachableTenorSpreadCurves.push_back(CurveType::_1M());
        cachableTenorSpreadCurves.push_back(CurveType::_3M());
        cachableTenorSpreadCurves.push_back(CurveType::_6M());
        cachableTenorSpreadCurves.push_back(CurveType::_1Y());

		std::string tenorSpreadCurveCacheTag;

        // for each cachable curve type...
        for(vector<CurveTypeConstPtr>::const_iterator iter(cachableTenorSpreadCurves.begin());
            iter != cachableTenorSpreadCurves.end(); ++iter)
        {
            tenorSpreadCurveCacheTag =  (*iter)->getDescription() + " Curve";

            // ... remember to cache the corresponding typed curve calculation (if added later) when:
            if(m_baseRate != *iter                                              // it makes sense
                && cacheTableExists                                             // to avoid crashing
                && cacheParametersTable->doesTagExist(tenorSpreadCurveCacheTag) // it's specified by the user
                && cacheParametersTable->get<long>(tenorSpreadCurveCacheTag , 0) != 0)
            {
                m_cachedCurveTypes.push_back(*iter);
            }
        }

        sort(m_cachedCurveTypes.begin(), m_cachedCurveTypes.end());
    }
	*/
    bool TenorSpreadSurface::useCurveCache(const CurveTypeConstPtr tenor) const
    {
        CurveTypeContainer::const_iterator lower(lower_bound(m_cachedCurveTypes.begin(),
                                                             m_cachedCurveTypes.end(),
                                                             tenor));

        return(lower != m_cachedCurveTypes.end() && (*lower == tenor));
    }

    TenorSpreadSurface::const_iterator TenorSpreadSurface::begin() const
    {
        return m_spreadCurves.begin();
    }

    TenorSpreadSurface::const_iterator TenorSpreadSurface::end() const
    {
        return m_spreadCurves.end();
    }

    vector<double> TenorSpreadSurface::getFlowTimes() const
    {
        return m_flowTimes;
    }

    bool TenorSpreadSurface::curveExists(const CurveTypeConstPtr& curveType) const
    {
        return m_spreadCurves.exists(curveType);
    }

    // Interpolate along an existing tenor
    double TenorSpreadSurface::interpolateCurve(const double tenor, const double flowTime) const
    {
		return m_tssInterpolation->interpolate(tenor, flowTime);
    }

    double TenorSpreadSurface::interpolate(const double tenor, const double flowTime) const
    {
		return m_tssInterpolation->interpolate(tenor, flowTime);
		/* Straight Line Interpolation:
        if(m_spreadCurves.size() < 2)
        {
            return 0.0;
        }

        const TypedCurves::const_iterator upper(upper_bound(m_spreadCurves.begin(),
                                                            m_spreadCurves.end(),
                                                            tenor,
                                                            YearFractionKeyValueCompare()));

        if(upper == m_spreadCurves.begin())
        {
            return upper->second->evaluate(flowTime);
        }

        const TypedCurves::const_iterator lower(upper - 1);

        if(upper == m_spreadCurves.end() || tenor == lower->first->getYearFraction())
        {
            return lower->second->evaluate(flowTime);
        }

        // linear interpolation between tenor curves evaluated at flowTime:
        const double lowerTenor(lower->first->getYearFraction());
        const double upperTenor(upper->first->getYearFraction());

        return ((tenor - lowerTenor) * upper->second->evaluate(flowTime) 
            +   (upperTenor - tenor) * lower->second->evaluate(flowTime) )  /  (upperTenor - lowerTenor);
			*/
    }

	void TenorSpreadSurface::accumulateCurveGradient(const CurveTypeConstPtr tenor,  
                                                     const double flowTime,
                                                     double multiplier, 
                                                     GradientIterator gradientBegin,
                                                     GradientIterator gradientEnd,
													 const CurveTypeConstPtr& curveType) const
    {
		m_tssInterpolation->accumulateGradient(tenor->getYearFraction(), flowTime, multiplier, gradientBegin, gradientEnd, curveType);
    }
    
    void TenorSpreadSurface::accumulateCurveGradient(const double tenor,  
                                                     const double flowTime,
                                                     double multiplier, 
                                                     GradientIterator gradientBegin,
                                                     GradientIterator gradientEnd,
													 const CurveTypeConstPtr& curveType) const
    {
		m_tssInterpolation->accumulateGradient(tenor, flowTime, multiplier, gradientBegin, gradientEnd, curveType);
    }
    
    void TenorSpreadSurface::accumulateGradient(const double tenor, 
                                                const double flowTime,
                                                double multiplier,
                                                GradientIterator gradientBegin,
                                                GradientIterator gradientEnd) const
    {
		m_tssInterpolation->accumulateGradient(tenor, flowTime, multiplier, gradientBegin, gradientEnd);
		/*	Tenor Straight Line Interpolation Gradient:
        if(m_spreadCurves.size() < 2)
        {
            return;
        }

        const TypedCurves::const_iterator upper(upper_bound(m_spreadCurves.begin(),
                                                            m_spreadCurves.end(),
                                                            tenor,
                                                            YearFractionKeyValueCompare()));

        if(upper == m_spreadCurves.begin())
        {
            upper->second->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
            return;
        }

        const TypedCurves::const_iterator lower(upper - 1);

        if(upper == m_spreadCurves.end() || tenor == lower->first->getYearFraction())
        {
            lower->second->accumulateGradient(flowTime, multiplier, gradientBegin, gradientEnd);
            return;
        }

        // linear interpolation between tenor curves evaluated at flowTime:
        const double lowerTenor(lower->first->getYearFraction());
        const double upperTenor(upper->first->getYearFraction());

        // Needs to know the number of unknowns on tenors curves BEFORE the one at lower
        const GradientIterator lowerStartPos(gradientBegin + getNumberOfUnknownsUpToTenor(lower->first->getYearFraction()));
        const GradientIterator upperStartPos(lowerStartPos + lower->second->getNumberOfUnknowns());

        lower->second->accumulateGradient(flowTime, 
                                          multiplier * (upperTenor - tenor) / (upperTenor - lowerTenor), 
                                          lowerStartPos, 
                                          upperStartPos);

        upper->second->accumulateGradient(flowTime, 
                                          multiplier * (tenor - lowerTenor) / (upperTenor - lowerTenor), 
                                          upperStartPos,
                                          upperStartPos + upper->second->getNumberOfUnknowns());
										  */

    }

    size_t TenorSpreadSurface::getNumberOfUnknowns() const
    {
        size_t ukpAcc(0);
        for(TypedCurves::const_iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
        {
            ukpAcc += iter->second->getNumberOfUnknowns();
        }
        return ukpAcc;
    }

	size_t TenorSpreadSurface::getNumberOfUnknowns(const CurveTypeConstPtr& tenor) const
	{
		if(tenor == CurveType::AllTenors())
		{
			return getNumberOfUnknowns();
		}
		else if(tenor == m_baseRate || !curveExists(tenor))
		{	
			return 0;
		}
		else
		{
			TypedCurves::const_iterator lower(m_spreadCurves.lowerBound(tenor));
			return(lower == m_spreadCurves.end() || lower->first !=  tenor ? 0 : lower->second->getNumberOfUnknowns());
		}
	}


    void TenorSpreadSurface::addKnotPoint(const CurveTypeConstPtr tenor, const KnotPoint& knotPoint)
    {
        checkIsTenor(tenor, "addKnotPoint");

        if(tenor == m_baseRate)
        {
            LT_THROW_ERROR("Can't add a knot-point to the spread curve over tenor");
        }
        else
        {

            TypedCurves::const_iterator lower(m_spreadCurves.lowerBound(tenor));

            if(lower == m_spreadCurves.end() || lower->first != tenor)
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
    }

    void TenorSpreadSurface::update()
    {
        m_totalNumberOfUnknownKnotPoints = 0;
        
        for(TypedCurves::iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
        {
            if(iter->first != m_baseRate)
            {
                iter->second->update();
                // there should be no unknown knot-points on the tenor spread curve over base rate:
                m_totalNumberOfUnknownKnotPoints += iter->second->getNumberOfUnknowns();   
            }
        }
    }

    void TenorSpreadSurface::finalize()
    {
        for(TypedCurves::iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
        {
			finalizeLogFvfCurve(*iter->second);
        }

		m_tssInterpolation->finalize();
		//	buildBucketedCurves();
    }

    void TenorSpreadSurface::addUnknownsToProblem(const LTQuant::ProblemPtr& problem)
    {
        for(TypedCurves::iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
        {
            if(iter->first != m_baseRate)    // if everything is safe elsewhere, this test should not be necessary
            {
                iter->second->addUnknownsToProblem(problem);
            }
        }
    }
	
	void TenorSpreadSurface::addUnknownsToProblem(const LTQuant::ProblemPtr& problem, 
												  IKnotPointFunctor& onKnotPointVariableAddedToProblem,
												  const CurveTypeConstPtr& curveType)
	{
		if(curveType == CurveType::AllTenors())
		{
			// Add all the unknowns on the tenor spread surface
			for(TypedCurves::iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
			{
				if(iter->first != m_baseRate)
				{
					iter->second->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
				}
			}
		} 
		else
		{
			// Add only the unknowns on the specified curve type
			TypedCurves::const_iterator iter(m_spreadCurves.lowerBound(curveType));
			if(iter != m_spreadCurves.end() && curveType == iter->first)
			{
				iter->second->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
			}
		}
	}

	void TenorSpreadSurface::updateVariablesFromShifts(const LTQC::VectorDouble::const_iterator shiftsBegin,
													  const LTQC::VectorDouble::const_iterator shiftsEnd)
	{
		// TO DO: check that shiftsEnd - shiftsBegin == # of variables on the surface
		size_t nbVariablesSoFar, tmp(0);
		for(TypedCurves::iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
		{
			nbVariablesSoFar = tmp;
			tmp += iter->second->getNumberOfUnknowns();

			// Note: must pass only the appropriate subset of the variable shifts!
			iter->second->updateVariablesFromShifts(shiftsBegin + nbVariablesSoFar, shiftsBegin + tmp);
		}
	}

    

	void TenorSpreadSurface::initializeKnotPoints(const CurveTypeConstPtr& tenor,
			 									  const double initialSpotRate)
	{
		if(tenor == CurveType::AllTenors())
		{
			// initialize all the curves with the same spot rate
			for(TypedCurves::const_iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
			{
				iter->second->initialize([initialSpotRate] (double x) {return multiTenorInitializationFunc(initialSpotRate, x);});
			}
		}
		else
		{
			// initialize only the curve of the specified tenor with the spot rate
			const TypedCurves::const_iterator iter(m_spreadCurves.lowerBound(tenor));
			if(iter != m_spreadCurves.end() && iter->first == tenor)
			{
				iter->second->initialize([initialSpotRate] (double x) {return multiTenorInitializationFunc(initialSpotRate, x);});
			}
		}
	}

	void TenorSpreadSurface::fillWithIndexSpineCurvesData(const LTQuant::GenericDataPtr& spineCurvesData) const
	{
		LTQuant::GenericDataPtr spineCurveData;

		for(TypedCurves::const_iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
		{
			if(iter->first != m_baseRate)
			{
				spineCurveData = iter->second->getCurveDetails();	
				spineCurvesData->set<LTQuant::GenericDataPtr>(iter->first->getDescription(), 0, spineCurveData);
				const size_t nbTenorBasisSpinePts(IDeA::numberOfRecords(*spineCurveData));
				for(size_t k(0); k < nbTenorBasisSpinePts; ++k)
				{				
					IDeA::inject<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, DF), k,
										 exp(-IDeA::extract<double>(*spineCurveData, IDeA_KEY(SPINECURVETABLE, Y), k)));
				}
			}
		}
	}

	void TenorSpreadSurface::getCurveInternalData(knot_points_container& kpc) const {
		for (auto p = begin(); p != end(); ++p)
			p->second->getCurveInternalData(kpc);
	}

	void TenorSpreadSurface::assignCurveInternalData(knot_points_container::const_iterator it) {
		for (auto p = begin(); p != end(); ++p)
			p->second->assignCurveInternalData(it++);
	}

    void TenorSpreadSurface::getUnfixedIndexSpineCurvesKnotPoints(std::list<double>& points) const
    {
        for(TypedCurves::const_iterator iter(m_spreadCurves.begin()), iterEnd(m_spreadCurves.end()); iter != iterEnd; ++iter)
        {
            if(iter->first != m_baseRate)
            {
                iter->second->getUnfixedKnotPoints(points);	
            }
        }
    }

    void TenorSpreadSurface::createTenorSpreadCurves(const LTQuant::GenericDataPtr curvesInterpolationTable,
                                                     const LeastSquaresResidualsPtr leastSquaresResiduals)
    {

        for(CurveType::Tenors::const_iterator tenorIter(CurveType::Tenors::begin());
            tenorIter != CurveType::Tenors::end();
            ++tenorIter)
        {
			LTQuant::GenericDataPtr interpolationDetailsTable;

            if((*tenorIter)->getDescription() != m_baseRate->getDescription())
			{
				IDeA::permissive_extract<LTQuant::GenericDataPtr>(curvesInterpolationTable, 
																  (*tenorIter)->getKey(),
																  interpolationDetailsTable);
				
				// Only insert the tenor spread curves for which a table is explicitly provided
                if(static_cast<bool>(interpolationDetailsTable))
                {
                    const ICurvePtr curve(ICurveFactory::createInstance(interpolationDetailsTable, leastSquaresResiduals));
                    m_spreadCurves.insert(*tenorIter, curve);
                }        
            }

        }
    }

    /// Returns the number of unknowns on the curves whose tenor is stricly less than the specified tenor
    size_t TenorSpreadSurface::getNumberOfUnknownsUpToTenor(const double tenor) const
    {
        size_t numberOfUnknowns(0);
        for(TypedCurves::const_iterator iter(m_spreadCurves.begin()); iter != m_spreadCurves.end(); ++iter)
        {
            if(iter->first->getYearFraction() < tenor)
            {
                numberOfUnknowns += iter->second->getNumberOfUnknowns();
            }
        }
        return numberOfUnknowns;
    }


    void TenorSpreadSurface::checkIsTenor(const CurveTypeConstPtr curveType,
										  const std::string& checkedFunctionName) const
    {
        if(!curveType->isTenor())
        {
            LT_THROW_ERROR("Error in function TenorSpreadSurface::" + checkedFunctionName 
                + ": the curve type '" + curveType->getDescription() + "' specified is not a tenor.");
        }
    }

    void TenorSpreadSurface::checkIsNotBaseRate(const CurveTypeConstPtr tenor,
                                                const std::string& checkedFunctionName,
                                                const std::string& errorMsg) const
    {
        if(tenor == m_baseRate)
        {
            LT_THROW_ERROR( "Error in function TenorSpreadSurface::" + checkedFunctionName + ": " + errorMsg );
        }
    }

    /* stand-alone (private) function?
    TenorSpreadSurface::CurveTypeContainer::const_iterator TenorSpreadSurface::getUpperBound(const double tenor) const
    {
        return upper_bound(m_spreadCurves.begin(),
                           m_spreadCurves.end(),
                           YearFractionKeyValueCompare());
    }
    */
    ostream& TenorSpreadSurface::print(ostream& out) const
    {
        out << "Tenor Spread Surface: " << endl;
        for(TypedCurves::const_iterator iter(m_spreadCurves.begin());
            iter != m_spreadCurves.end();
            ++iter)
        {
            iter->second->print(out);
            out << endl;
        }
        return out;
    }

    bool TenorSpreadSurface::YearFractionKeyValueCompare::operator()(const KeyValuePair& lhs, const KeyValuePair& rhs) const
    {
        return operator()(lhs.first->getYearFraction(), rhs.first->getYearFraction());
    }
    
    bool TenorSpreadSurface::YearFractionKeyValueCompare::operator()(const KeyValuePair& lhs, const double rhs) const
    {
        return operator()(lhs.first->getYearFraction(), rhs);
    }

    bool TenorSpreadSurface::YearFractionKeyValueCompare::operator()(const double lhs, const KeyValuePair& rhs) const
    {
        return operator()(lhs, rhs.first->getYearFraction());
    }

    bool TenorSpreadSurface::YearFractionKeyValueCompare::operator()(const double lhs, const double rhs) const
    {
        return lhs < rhs;
    }

}