/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "UkpCurve.h"
#include "InterpolationMethod.h"
#include "InterpolationMethodFactory.h"
#include "KnotPoints.h"
#include "StraightLineInterpolation.h"
#include "Data\GenericData.h"
#include "StubBaseModel.h"
#include "LeastSquaresResiduals.h"
#include "Maths/Problem.h"
#include "SolverVariable.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"


using namespace LTQC;
using namespace std;

namespace FlexYCF
{
    UkpCurve::UkpCurve()
    {
    }

    UkpCurve::UkpCurve(const KnotPointsPtr knotPoints,
                       const InterpolationMethodPtr interpolationMethod):
        m_interpolationMethod(interpolationMethod)
    {
        setKnotPoints(knotPoints);
    }


    InterpolationCurvePtr UkpCurve::createInstance(const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                   const KnotPointsPtr knotPoints,
                                                   const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
        static const string defaultInterpolationMethodName(StraightLineInterpolation::getName());
        
        // Retrieve the name of the interpolation method to use
        string interpolationMethodName(defaultInterpolationMethodName);
        if(static_cast<bool>(interpolationCurveDetailsTable))
        {
			IDeA::permissive_extract<string>(*interpolationCurveDetailsTable, 
											 IDeA_KEY(INTERPOLATIONCURVEDETAILS, INTERP),
											 interpolationMethodName,
											 defaultInterpolationMethodName);
		}

        // Create the UkpCurve
        const UkpCurvePtr ukpCurve(new UkpCurve);
		LT_LOG << "Creating UkpCurve with name " << interpolationMethodName;
        ukpCurve->m_interpolationMethod = InterpolationMethodFactory::create(interpolationMethodName);
        
        // Sets the  knot-koints
        ukpCurve->setKnotPoints(knotPoints);
        
        return ukpCurve;
    }

    InterpolationCurvePtr UkpCurve::create(const LTQuant::GenericDataPtr interpolationCurveDetailsTable)
    {
        LeastSquaresResidualsPtr lsr(new LeastSquaresResiduals(BaseModelPtr(new StubBaseModel(0.05))));
        return createInstance(interpolationCurveDetailsTable, 
                              KnotPointsPtr(), 
                              lsr);
    }

    double UkpCurve::interpolate(const double x) const
    {
        return m_interpolationMethod->evaluate(x);
    }


    void UkpCurve::accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        m_interpolationMethod->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);
    }

	void UkpCurve::addKnotPointVariableToProblem(const size_t kpIndex,
						 						 const LTQuant::ProblemPtr& problem)
	{
		// only add the y of a knot-point as a problem variable if the knot-point is marked as unknown  
		if(!getKnotPoint(kpIndex).isKnown)
		{
			problem->addVariable(SolverVariablePtr(new SolverVariable(getKnotPoint(kpIndex).y)));
		}
	}

    void UkpCurve::finalize()
    {
        m_interpolationMethod->setExtremalIterators(begin(), end());
    }

    void UkpCurve::update()
    {
		m_interpolationMethod->update();
    }

    size_t UkpCurve::getNumberOfUnknowns() const
    {
        return getNumberOfUnknownKnotPoints();
    }

	double UkpCurve::getUnknown(const size_t index) const
	{
		// Encapsulate this in a double& getUnknownY function:
		size_t nbUnknownsSoFar(0);

		KnotPoints::const_iterator iter(InterpolationCurve::begin());
		for( ; ; ++iter)
		{
			if(!iter->isKnown)
			{
				++nbUnknownsSoFar;
			}
			if(index + 1 == nbUnknownsSoFar)
			{
				break;
			}
		}
		return iter->y;
	}

	void UkpCurve::setUnknown(const size_t index, const double value)
	{
		// Encapsulate this in a double& getUnknownY function:
		size_t nbUnknownsSoFar(0);
		for(size_t k(0); ; ++k)
		{
			if(!getKnotPoint(k).isKnown)
			{
				++nbUnknownsSoFar;
			}
			if(index + 1 == nbUnknownsSoFar)
			{
				getKnotPoint(k).y = value;
                return;
			}
		}
	}

    std::ostream& UkpCurve::print(std::ostream& out) const
    {
        out << "UkpCurve with interp " << (*m_interpolationMethod) << endl; 
        return out;
    }

    /**
        @brief Create a clone.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param lookup A lookup of previously created clones.
        @return       A clone of this instance.
    */
    ICloneLookupPtr UkpCurve::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new UkpCurve(*this, lookup));
    }

    /**
        @brief A pseudo copy constructor.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param original The original instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    UkpCurve::UkpCurve(UkpCurve const& original, CloneLookup& lookup) :
        InterpolationCurve(original, lookup),
        m_interpolationMethod(original.m_interpolationMethod->clone())
    {
        // Invoke finalise so we ensure that iterator member data in derived classes is setup correctly
        finalize();
    }
}