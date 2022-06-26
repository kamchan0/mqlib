#include "stdafx.h"
#include "SingleCurveDefaultKpp.h"
#include "BaseModel.h"
#include "SingleCurveModel.h"
#include "KnotPoint.h"


using namespace LTQC;

namespace FlexYCF
{
    /// Not sure this can be considered a default KPP for single curve as getLastRelevantTime() 
    /// returns 1.0 for all instruments except ILZCSwapInstrument, which will create an error
    /// as soon as 1.0 is tried to be added twice on the curve.
    bool SingleCurveDefaultKpp::createKnotPoints(const CalibrationInstruments& instruments, 
                                                 const BaseModelPtr baseModel)
    {
		const SingleCurveModelPtr singleCurve(std::tr1::dynamic_pointer_cast<SingleCurveModel>(baseModel));

        for(size_t i(0); i < instruments.size(); ++i)
        {
            KnotPoint kp(instruments[i]->getLastRelevantTime(), 200.0, false, instruments[i]);
            singleCurve->addKnotPoint(kp);
        }
  
		// The update does not seem necessary, and will fail the MCS interpolation.
		// TODO: To test whether commenting out the following line would create problem.
//         baseModel->update();

        return true;
    }
    
}