/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"

//	FlexYCF
#include "CompositeCurve.h"
#include "InterpolationCurveFactory.h"
#include "LeastSquaresResiduals.h"
#include "Futures.h"
#include "FlexYCFCloneLookup.h"

//	LTQuantLib
#include "Data\GenericData.h"

//	IDeA
#include "DataExtraction.h"
#include "DictYieldCurve.h"


using namespace std;

using namespace LTQC;

namespace FlexYCF
{

    string CompositeCurve::getName()
    {
        return "Composite";
    }

    InterpolationCurvePtr CompositeCurve::createInstance(const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                         const KnotPointsPtr knotPoints,
                                                         const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
        LTQuant::GenericDataPtr leftInterpolationCurveDetailsTable, rightInterpolationCurveDetailsTable;

        const double defaultSeparationX(0.0);
        double separationX(defaultSeparationX);
		IDeA::CompositeSeparationType separationType;
		LT::Str separationTypeStr;

        // Retrieve the separationX from the table, or alternatively the separation type
        if(static_cast<bool>(interpolationCurveDetailsTable))
        {
			IDeA::permissive_extract<LTQuant::GenericDataPtr>(*interpolationCurveDetailsTable,
															  IDeA_KEY(INTERPOLATIONCURVEDETAILS, LEFTCURVE),
															  leftInterpolationCurveDetailsTable);

            IDeA::permissive_extract<LTQuant::GenericDataPtr>(*interpolationCurveDetailsTable,
															  IDeA_KEY(INTERPOLATIONCURVEDETAILS, RIGHTCURVE),
															  rightInterpolationCurveDetailsTable);

			//	Sets the separation point, remembering whether the default value is used
			//	Note:
			//	For SEPARATION tag is not used to assign a value, the function setSeparationpoint is called in the onKnotPointsPlaced function of concrete models
			if (IDeA::permissive_extract<double>(*interpolationCurveDetailsTable, IDeA_KEY(INTERPOLATIONCURVEDETAILS, SEPARATION), separationX, defaultSeparationX))
			{
				separationType = IDeA::CompositeSeparationType::Assigned;
			}
			else if (IDeA::permissive_extract<LT::Str>(interpolationCurveDetailsTable->table, IDeA_KEY(INTERPOLATIONCURVEDETAILS, SEPARATIONTYPE), separationTypeStr))
			{
				separationX = defaultSeparationX;
				separationType = IDeA::CompositeSeparationType(separationTypeStr);
			}
			else
			{
				separationX = defaultSeparationX;
				separationType = IDeA::CompositeSeparationType::Default;
			}
        }
        
        CompositeCurvePtr compositeCurve(new CompositeCurve);
        
        compositeCurve->m_leftCurve					= InterpolationCurveFactory::createInstance(leftInterpolationCurveDetailsTable, knotPoints, leastSquaresResiduals);
        compositeCurve->m_rightCurve				= InterpolationCurveFactory::createInstance(rightInterpolationCurveDetailsTable, knotPoints, leastSquaresResiduals);
        compositeCurve->m_separationX				= separationX;
		compositeCurve->m_separationType			= separationType;
        compositeCurve->m_leastSquaresResiduals		= leastSquaresResiduals;

        compositeCurve->setKnotPoints(knotPoints);

        return compositeCurve;
    }

    double CompositeCurve::interpolate(const double x) const
    {
        if(x <= m_separationX)
        {
            return m_leftCurve->interpolate(x);
        }
        else
        {
            return m_rightCurve->interpolate(x);
        }
    }

    void CompositeCurve::accumulateGradient(const double x, double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const
    {
        if(x <= m_separationX)
        {
            m_leftCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);
        }
        else
        {
            m_rightCurve->accumulateGradient(x, multiplier, gradientBegin, gradientEnd);
        }
    }
    
	/*
	// Obsolete: to be replaced with addKnotPointVariableToProblem
	void CompositeCurve::addUnknownsToProblem(const LTQuant::ProblemPtr& problem)
    {
        // add unknowns of each of the underlying curve up to and from the separation knot 
        m_leftCurve->addUnknownsToProblem(problem);
    }
	*/

	void CompositeCurve::addKnotPointVariableToProblem(const size_t kpIndex,
													   const LTQuant::ProblemPtr& problem)
	{
		m_leftCurve->addKnotPointVariableToProblem(kpIndex, problem);
	}

    void CompositeCurve::finalize()
    {
        m_leftCurve->finalize();
        m_rightCurve->finalize();
    }

    void CompositeCurve::update()
    {
        m_leftCurve->update();
        for(size_t i(0); i < m_leftCurve->size(); ++i)
        {
            m_rightCurve->getKnotPoint(i).y = m_leftCurve->getKnotPoint(i).y;
        }
        m_rightCurve->update();
    }

    size_t CompositeCurve::getNumberOfUnknowns() const
    {
        return m_leftCurve->getNumberOfUnknowns();
    }

	double CompositeCurve::getUnknown(const size_t index) const
	{
		return m_leftCurve->getUnknown(index);
	}

	void CompositeCurve::setUnknown(const size_t index, const double value)
	{
		m_rightCurve->setUnknown(index, value);
		m_leftCurve->setUnknown(index, value);
	}

    void CompositeCurve::onKnotPointAdded(const KnotPoint& knotPoint)
    {
        m_leftCurve->onKnotPointAdded(knotPoint);
        m_rightCurve->onKnotPointAdded(knotPoint);
    }

    std::ostream& CompositeCurve::print(std::ostream& out) const
    {
        out << "Composite Curve" << endl;
        out << " Left Curve: " << (*m_leftCurve) << endl;
        out << "Right Curve: " << (*m_rightCurve) << endl;
        out << "Separation : " << m_separationX << endl;
        return out;
    }

    /**
        @brief Create a clone.

        Uses a lookup to ensure that the directed graph relationships are maintained in the clone.

        @param lookup The lookup of previously created clones.
        @return       The clone of this instance.
    */
    ICloneLookupPtr CompositeCurve::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new CompositeCurve(*this, lookup));
    }

    /**
        @brief A pseudo copy constructor using lookup.

        Uses a lookup to ensure that directed graph relationships are maintained in the clone.

        @param original The original instances.
        @param lookup   A lookup of previously created clones.
    */
    CompositeCurve::CompositeCurve(CompositeCurve const& original, CloneLookup& lookup) :
        InterpolationCurve(original, lookup),
        m_leftCurve(lookup.get(original.m_leftCurve)),
        m_rightCurve(lookup.get(original.m_rightCurve)),
        m_separationX(original.m_separationX),
        m_separationType(original.m_separationType),
        m_leastSquaresResiduals(lookup.get(original.m_leastSquaresResiduals))
    {
    }

    CompositeCurve::CompositeCurve()
    {
    }
}