/*****************************************************************************

    BaseKnotPointPlacement
    
	Interface of all knot-point placements.


    @Originator		BaseKnotPointPlacement
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BASEKNOTPOINTPLACEMENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BASEKNOTPOINTPLACEMENT_H_INCLUDED
#pragma once


//	FlexYCF
#include "CalibrationInstruments.h"


namespace FlexYCF
{
	class LeastSquaresResiduals;

    FWD_DECLARE_SMART_PTRS( BaseModel )

    /// BaseKnotPointPlacement is an abstract class that exposes an
    /// interface common to all knot-points placement algorithms.
    /// It encapsulates the knot-point placement behaviour of a model
    /// that is calibrated. 
    class BaseKnotPointPlacement
    {
    public:
        virtual ~BaseKnotPointPlacement() = 0 
        {
        }

        /// Selects a subset of the specified instruments to use
        /// to place knots
        virtual void selectInstruments(CalibrationInstruments& /* instruments */, 
                                       const BaseModelPtr /* baseModel */)
        {
            /// Do nothing by default
        }

        /// Creates the knot-points and return whether the placement has succeeded
        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel) = 0;

		virtual void onLeastSquaresResidualsAdded(LeastSquaresResiduals&) const
		{
			// Do nothing by default
		}
    };

    DECLARE_SMART_PTRS( BaseKnotPointPlacement )
}
#endif //__LIBRARY_PRICERS_FLEXYCF_BASEKNOTPOINTPLACEMENT_H_INCLUDED