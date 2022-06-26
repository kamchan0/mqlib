/*****************************************************************************
	FlexYcfUtils
    
	
	@Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"


//	FlexYCF
#include "FlexYcfUtils.h"

//	LTQuantCore
#include "Matrix.h"


using namespace LTQC;

namespace FlexYCF
{
	std::string date_to_string(const LT::date& date_)
	{
        // This outputs according to the format of %Y-%b-%d although the old boost one looks like %d/%b/%Y
        // because the boost one actually produces a date according to %Y-%b-%d, i.e. it 
        // seems like the boost time_facet imbueing might be being used incorrectly.
        // The test QLT/IDeATest.Functors/201 uses this (and fails otherwise)
        return DateFunctional::toSimpleString(date_.getAsLong());
	}
	
	std::ostream& operator<<(std::ostream& out, const LTQC::Matrix& mat)
	{
		for(size_t i(0); i < mat.getNumRows(); ++i)
		{
			for(size_t j(0); j < mat.getNumCols(); ++j)
				out << mat(i,j) << '\t';
			out << std::endl;
		}
		return out;
	}
}