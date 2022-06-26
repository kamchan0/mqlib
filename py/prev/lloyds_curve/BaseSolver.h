/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_BASESOLVER_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_BASESOLVER_H_INCLUDED
#pragma once

// FlexYCF
#include "CalibrationInstruments.h"

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BaseModel )

    /// An abstract class that exposes an interface that all solvers
    /// in the FlexYCF must implement.
    class BaseSolver
    {
    protected:
        virtual ~BaseSolver() = 0 { }
        
    public:
        enum State
        {
            FIRST_SOLVING,
            REFRESHING
        };

        /// Calibrates the model to the instruments
        virtual void solve(const CalibrationInstruments& calibrationInstruments, 
                           const BaseModelPtr baseModel) = 0;
 
        void setState(const State state)
        {
            m_state = state;
        }

    private:
        State m_state;
    };  //  BaseSolver

    DECLARE_SMART_PTRS( BaseSolver )

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_BASESOLVER_H_INCLUDED