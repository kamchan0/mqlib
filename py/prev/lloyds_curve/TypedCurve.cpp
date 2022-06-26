#include "stdafx.h"
#include "TypedCurve.h"
#include "MultiTenorModel.h"
#include "SolverVariable.h"


using namespace LTQC;

namespace FlexYCF
{
    
    TypedCurve::TypedCurve(const CurveTypeConstPtr curveType,
                           const bool addZeroFixedKnotPoint) :
        // BaseCurve(new KnotPoints, new InterpolationCurve, new LeftExtrapolation, new RightExtrapolation),
        m_curveTypePtr(curveType)
    { 
        if(addZeroFixedKnotPoint)
        {
            addKnotPoint(KnotPoint(0.0, 0.0, true));    
        }
    }
    /*
    string TypedCurve::getName()
    {
        return "TypedCurve";
    }
        
    ICurvePtr TypedCurve::createInstance(const LTQuant::GenericDataPtr interpolationDetailsTable,
                                         const LeastSquaresResidualsPtr leastSquaresResiduals)
    {

        TypedCurvePtr typedCurve(new TypedCurve(interpolationDetailsTable));
        return typedCurve;
    }
    */
    CurveTypeConstPtr TypedCurve::getCurveType() const
    {
        return m_curveTypePtr;
    }

    ostream& TypedCurve::print(ostream& out) const
    {
        out << m_curveTypePtr->getDescription() << endl;
        return out;
    }
}