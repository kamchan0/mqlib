/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "BaseCurve.h"
#include "InterpolationCurve.h"
#include "ExtrapolationMethodFactoryDefs.h"
#include "ExtrapolationMethods.h"
#include "InterpolationCurveFactory.h"
#include "KnotPoints.h"
#include "Data\GenericData.h"
#include "UkpCurve.h"
#include "CalibrationInstrument.h"
#include "FlexYCFCloneLookup.h"

#include "DataExtraction.h"
#include "DictYieldCurve.h"

using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    BaseCurve::BaseCurve()
    {
    }
    
    BaseCurve::BaseCurve(const KnotPointsPtr knotPoints,
                         const InterpolationCurvePtr interpolationCurve,
                         const LeftExtrapolationPtr leftExtrapolationMethod,
                         const RightExtrapolationPtr rightExtrapolationMethod):
        m_knotPoints(knotPoints),
        m_interpolationCurve(interpolationCurve),
        m_leftExtrapolationMethod(leftExtrapolationMethod),
        m_rightExtrapolationMethod(rightExtrapolationMethod)
    {
		m_leftExtrapolationMethod->setInterpolationGradientFunction(
			[interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd) 
						{return interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);});
        m_rightExtrapolationMethod->setInterpolationGradientFunction(
			[interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd) 
						{return interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);});
    }
    
    string BaseCurve::getName()
    {
        return "BaseCurve";
    }
        
    ICurvePtr BaseCurve::createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable,
                                        const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
        const string defaultInterpolationCurveTypeName(UkpCurve::getName());
        const string defaultLeftExtrapolationMethodName(LeftFlatExtrapolationMethod::getName());
        const string defaultRightExtrapolationMethodName(RightStraightLineExtrapolationMethod::getName());

        // Create KnotPoints object
        const KnotPointsPtr knotPoints(new KnotPoints);

        // Create InterpolationCurve from table
        LTQuant::GenericDataPtr interpolationCurveDetailsTable;
        if(static_cast<bool>(interpolationDetailsTable))
        {
			IDeA::permissive_extract<LTQuant::GenericDataPtr>(*interpolationDetailsTable, 
															  IDeA_KEY(INTERPOLATIONDETAILS, INTERPOLATIONCURVEDETAILS),
															  interpolationCurveDetailsTable);
		}

        string interpolationCurveTypeName(defaultInterpolationCurveTypeName);
        if(static_cast<bool>(interpolationCurveDetailsTable))
        {
			IDeA::permissive_extract<string>(*interpolationCurveDetailsTable, 
											 IDeA_KEY(INTERPOLATIONCURVEDETAILS, CURVETYPE),
											 interpolationCurveTypeName,
											 defaultInterpolationCurveTypeName);
		} 

        InterpolationCurvePtr interpolationCurve(InterpolationCurveFactory::createInstance(interpolationCurveTypeName, 
                                                                                           interpolationCurveDetailsTable,
                                                                                           knotPoints,
                                                                                           leastSquaresResiduals));

        // Create LeftExtrapolationMethod
        string leftExtrapolationMethodName(defaultLeftExtrapolationMethodName);
        if(static_cast<bool>(interpolationDetailsTable))
        {
			IDeA::permissive_extract<string>(*interpolationDetailsTable, 
											 IDeA_KEY(INTERPOLATIONDETAILS, LEFTEXTRAP),
											 leftExtrapolationMethodName,
											 defaultLeftExtrapolationMethodName);
		}
        LeftExtrapolationPtr leftExtrapolationMethod    (
            LeftExtrapolationMethodFactory::createInstance(
				leftExtrapolationMethodName, 
				[interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd) 
						{interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}
			)
        );

        // Create RigthExtrapolationMethod
        string rightExtrapolationMethodName(defaultRightExtrapolationMethodName);
        if(static_cast<bool>(interpolationDetailsTable))
        {
			IDeA::permissive_extract<string>(*interpolationDetailsTable,
											 IDeA_KEY(INTERPOLATIONDETAILS, RIGHTEXTRAP),
											 rightExtrapolationMethodName,
											 defaultRightExtrapolationMethodName);
		}
        RightExtrapolationPtr rightExtrapolationMethod  (
            RightExtrapolationMethodFactory::createInstance(
			    rightExtrapolationMethodName,
				[interpolationCurve] (double x, double multiplier, GradientIterator& gradientBegin, GradientIterator& gradientEnd) 
						{interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);}
			)
        );

        // Create the BaseCurve and sets the interpolation curve and the extrapolation methods
        BaseCurvePtr baseCurve(new BaseCurve);

        baseCurve->m_knotPoints = knotPoints;   // those knot-points were passed to the InterpolationCurve

        baseCurve->m_interpolationCurve = interpolationCurve;
        baseCurve->m_leftExtrapolationMethod = leftExtrapolationMethod;
        baseCurve->m_rightExtrapolationMethod = rightExtrapolationMethod;
        
        return ICurvePtr(baseCurve);
    }

    ICurvePtr BaseCurve::create(const LTQuant::GenericDataPtr interpolationDetailsTable)
    {
        return BaseCurve::createInstance(interpolationDetailsTable, LeastSquaresResidualsPtr());
    }

    // considers the intervals [t[i-1[, t[i]) (because of upperbound)
    double BaseCurve::evaluate(const double x) const
    {
        if(x <= m_knotPoints->xMin())
        {   //  Left extrapolation
            return m_leftExtrapolationMethod->evaluate(x);
        }
        else if(x < m_knotPoints->xMax())
        {   //  Interpolation
            return m_interpolationCurve->interpolate(x);
        }
        //  Right extrapolation
        return m_rightExtrapolationMethod->evaluate(x);
    }

    /// Computes the gradient of the curve function at 
    /// x relative to its unknowns
    void BaseCurve::accumulateGradient(const double x, 
                                       double multiplier,
                                       GradientIterator gradientBegin,
                                       GradientIterator gradientEnd) const
    {
        if(x <= m_knotPoints->xMin())
        {   //  Left extrapolation
            m_leftExtrapolationMethod->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);
        }
        else if(x < m_knotPoints->xMax())
        {    //  Interpolation
            m_interpolationCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);
        }
        else 
        {    //  Right extrapolation
            m_rightExtrapolationMethod->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);
        }
    }

    void BaseCurve::addKnotPoint(const KnotPoint& knotPoint)
    {
        m_interpolationCurve->addKnotPoint(knotPoint);
    }

    void BaseCurve::addUnknownsToProblem(const LTQuant::ProblemPtr& problem)
    {
        m_interpolationCurve->addUnknownsToProblem(problem);
    }

	void BaseCurve::addUnknownsToProblem(const LTQuant::ProblemPtr& problem,
										 IKnotPointFunctor& onKnotPointVariableAddedToProblem)
	{
		m_interpolationCurve->addUnknownsToProblem(problem, onKnotPointVariableAddedToProblem);
	}

	void BaseCurve::updateVariablesFromShifts(const LTQC::VectorDouble::const_iterator shiftsBegin,
											 const LTQC::VectorDouble::const_iterator shiftsEnd)
	{
		LTQC::VectorDouble::const_iterator iter(shiftsBegin);
		for(size_t k(0); k < getNumberOfUnknowns(); ++k)
		{
            if( *iter != 0.0 )
            {
			    setUnknown(k, getUnknown(k) + *iter);
            }
            ++iter;
		}
	}

    void BaseCurve::finalize()
    {    
        // Finalize the interpolation curve first
        m_interpolationCurve->finalize();
        m_leftExtrapolationMethod->setExtremalIterators(m_knotPoints->begin(), m_knotPoints->end());
        m_rightExtrapolationMethod->setExtremalIterators(m_knotPoints->begin(), m_knotPoints->end());   
    }

    void BaseCurve::update()
    {
        m_interpolationCurve->update();
        // Note: a priori no need to update extrapolation methods
    }

    size_t BaseCurve::getNumberOfUnknowns() const
    {
        return m_interpolationCurve->getNumberOfUnknowns();
    }

	double BaseCurve::getUnknown(const size_t index) const
	{
		return m_interpolationCurve->getUnknown(index);
	}

	void BaseCurve::setUnknown(const size_t index, const double value)
	{
		m_interpolationCurve->setUnknown(index, value);
	}

	size_t BaseCurve::getNumberOfKnots() const
	{
		return m_interpolationCurve->size();
	}

	LTQuant::GenericDataPtr BaseCurve::getCurveDetails() const
	{
		const LTQuant::GenericDataPtr curveDetailsData(new LTQuant::GenericData("Knot-points", 0));
		const std::string na("N.A.");

		KnotPoint* tmpKp;
		for(size_t k(0); k < m_knotPoints->size(); ++k)
		{
			tmpKp = &((*m_knotPoints)[k]);
			
			IDeA::inject<long>(*curveDetailsData, IDeA_KEY(SPINECURVETABLE, NUMBER), k, static_cast<long>(k));
			IDeA::inject<std::string>(*curveDetailsData, IDeA_KEY(SPINECURVETABLE, INSTRUMENT), k, (tmpKp->getRelatedInstrument()? tmpKp->getRelatedInstrument()->getType(): na));
			IDeA::inject<std::string>(*curveDetailsData, IDeA_KEY(SPINECURVETABLE, MATURITY), k, (tmpKp->getRelatedInstrument()? tmpKp->getRelatedInstrument()->getDescription().string(): na));
			IDeA::inject<double>(*curveDetailsData, IDeA_KEY(SPINECURVETABLE, X), k, tmpKp->x);
			IDeA::inject<double>(*curveDetailsData, IDeA_KEY(SPINECURVETABLE, Y), k, tmpKp->y);
			IDeA::inject<long>(*curveDetailsData, IDeA_KEY(SPINECURVETABLE, FIXED), k, tmpKp->isKnown);
		}

		return curveDetailsData;
	}

	void BaseCurve::getCurveInternalData(knot_points_container& kpc) const {
		kpc.push_back(std::vector<std::pair<double, double>>());
		std::vector<std::pair<double, double>>& v = kpc.back();
		for (auto p = m_knotPoints->begin(); p != m_knotPoints->end(); ++p)
			v.push_back(make_pair(p->x, p->y));
	}

	void BaseCurve::assignCurveInternalData(knot_points_container::const_iterator it) {
		for (size_t i = 0; i < m_knotPoints->size(); ++i) {
			(*m_knotPoints)[i].x = (*it)[i].first;
			(*m_knotPoints)[i].y = (*it)[i].second;
		}
	}

    void BaseCurve::getUnfixedKnotPoints(std::list<double>& points) const
    {
        KnotPoint* tmpKp;
        for(size_t k=0, m=m_knotPoints->size(); k < m; ++k)
        {
            tmpKp = &((*m_knotPoints)[k]);

            if(!tmpKp->isKnown)
                points.push_back(tmpKp->x);
        }
    }

    ostream& BaseCurve::print(ostream& out) const
    {
        out << "Interp Curve: " << (*m_interpolationCurve) << endl;
        // TO DO
        // out << "Left Extrap: " << ..
        return out;
    }

    KnotPoints::const_iterator BaseCurve::begin() const
    {
        return m_knotPoints->begin();
    }
    
    KnotPoints::const_iterator BaseCurve::end() const
    {
        return m_knotPoints->end();
    }   
	/*
    void BaseCurve::initializeKnotPoints(const KnotPoints::InitFunction& initFunction)
    {
        m_knotPoints->setYsFromXsWithFunction(initFunction);
    }
	*/
	void BaseCurve::initialize(const CurveInitializationFunction& curveInitializationFunction)
	{
		m_knotPoints->setUnknownYsFromXsWithFunction(curveInitializationFunction);
	}

	void BaseCurve::onKnotPointsInitialized() const
	{
		m_interpolationCurve->onKnotPointsInitialized();
	}

    /**
        @brief Clone this instance.

        Use a lookup to ensure directed graph relationships are maintained in the clone.

        @param lookup The lookup of previously created clones.
        @return       Returns the clone.
    */
    ICloneLookupPtr BaseCurve::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new BaseCurve(*this, lookup));
    }

    /**
        @brief Pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained in the clone.

        @param original The original instance to copy from.
        @param lookup   The lookup of previously created clones.
    */
    BaseCurve::BaseCurve(BaseCurve const& original, CloneLookup& lookup) :
        m_knotPoints(lookup.get(original.m_knotPoints)),
        m_interpolationCurve(lookup.get(original.m_interpolationCurve)),
        m_leftExtrapolationMethod(original.m_leftExtrapolationMethod->clone()),
        m_rightExtrapolationMethod(original.m_rightExtrapolationMethod->clone())
    {
        // The clone still has iterators that refer to m_knotPoints in the original BaseCurve. We finalize here to ensure
        // these iterators are updated.
        finalize();
    }

    void  BaseCurve::setInterpolationCurve(const InterpolationCurvePtr interpolationCurve)
    {
        m_interpolationCurve = interpolationCurve;
        m_interpolationCurve->finalize();
    }
    
    void  BaseCurve::setLeftExtrapolationMethod(const LeftExtrapolationPtr leftExtrapolationMethod)
    {
        m_leftExtrapolationMethod = leftExtrapolationMethod;
        m_leftExtrapolationMethod->setExtremalIterators(m_knotPoints->begin(), m_knotPoints->end());
    }
    
    void  BaseCurve::setRightExtrapolationMethod(const RightExtrapolationPtr rightExtrapolationMethod)
    {
        m_rightExtrapolationMethod = rightExtrapolationMethod;
        m_rightExtrapolationMethod->setExtremalIterators(m_knotPoints->begin(), m_knotPoints->end());
    }

}

