/*****************************************************************************

    SolverVariable

	Represents the variable of a solver in FlexYCF
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_SOLVERVARIABLE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_SOLVERVARIABLE_H_INCLUDED
#include "Maths\Variable.h"


namespace FlexYCF
{

    class SolverVariable : public LTQuant::Variable
    {
    public:
        explicit SolverVariable(double& value): 
            LTQuant::Variable(value, value) //, m_isBaseRate(isBaseRate), m_timeToMaturity(timeToMaturity)
        {  }

        virtual bool    isValid() const { return true; }
        virtual double residual() const { return 0.0;  }

    };  // SolverVariable


    DECLARE_SMART_PTRS( SolverVariable )

}   // FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_SOLVERVARIABLE_H_INCLUDED