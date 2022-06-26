/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_DONOTHINGSOLVER_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_DONOTHINGSOLVER_H_INCLUDED

#include "BaseSolver.h"


namespace LTQUant
{
	FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{
	// A solver that does nothing
	class DoNothingSolver : public BaseSolver
	{
	public:
		static std::string getName()
		{
			return "DoNothing";
		}

		static BaseSolverPtr createInstance(const LTQuant::GenericDataPtr&)
		{
			return BaseSolverPtr(new DoNothingSolver);
		}

        virtual void solve(const CalibrationInstruments&, 
						   const BaseModelPtr)
		{
			// do nothing
		}
	};
	
	DECLARE_SMART_PTRS( DoNothingSolver )

}	//	FlexYCF

#endif	//	__LIBRARY_PRICERS_FLEXYCF_DONOTHINGSOLVER_H_INCLUDED