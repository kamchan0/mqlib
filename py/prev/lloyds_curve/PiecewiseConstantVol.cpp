#include "stdafx.h"
#include "PiecewiseConstantVol.h"
#include "ICurve.h"
#include "CurveCreationHelper.h"
#include "StraightLineInterpolation.h"
#include "ExtrapolationMethods.h"
#include "KnotPoint.h"
#include "NullDeleter.h"

using namespace std;

using namespace LTQC;

namespace LTQuant
{
    PiecewiseConstantVol::PiecewiseConstantVol():
        m_constantVols(FlexYCF::createUkpCurve<FlexYCF::StraightLineInterpolation, 
                                               FlexYCF::LeftFlatExtrapolationMethod, 
                                               FlexYCF::RightFlatExtrapolationMethod>())
    {
    }

    LTQuant::InstantaneousVolPtr PiecewiseConstantVol::create(const std::vector<double>& tenorStructure)
    {
        PiecewiseConstantVolPtr piecewiseConstantVol(new PiecewiseConstantVol);

        const double defaultInitVol(0.20); // could be a param
        for(size_t pt(0); pt < tenorStructure.size(); ++pt)
        {
            piecewiseConstantVol->m_constantVols->addKnotPoint(FlexYCF::KnotPoint(tenorStructure[pt], defaultInitVol, false));
        }

        piecewiseConstantVol->m_constantVols->finalize();

        return piecewiseConstantVol;
    }
        
    double PiecewiseConstantVol::getIntegralSigmaSigma(double startTime, double endTime, double ti, double tj) const
    {
        return (endTime - startTime) * m_constantVols->evaluate(ti) * m_constantVols->evaluate(tj);
    }

    double PiecewiseConstantVol::getSigma(double /* time */, double ti) const
    {
        return m_constantVols->evaluate(ti);
    }

    DevCore::Properties PiecewiseConstantVol::getProperties()
    {
        DevCore::Properties properties;
        // TO DO?
        return properties;
    }

    void PiecewiseConstantVol::addToProblem(Problem& problem)
    {
        m_constantVols->addUnknownsToProblem(ProblemPtr(&problem, FlexYCF::NullDeleter()));
    }

    void PiecewiseConstantVol::onUpdate()
    {
        // do nothing
    }

    string PiecewiseConstantVol::getName()
    {
        return "PiecewiseConstant";
    }
}