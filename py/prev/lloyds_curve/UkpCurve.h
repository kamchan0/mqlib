/*****************************************************************************

    UkpCurve

	Handles usual interpolation methods as functions that, given a set of
	knot-points, goes through those points
	
	Note: UKP stands for Unknown Knot-Point
    

    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_UKPCURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_UKPCURVE_H_INCLUDED
#pragma once
#include "InterpolationCurve.h"
#include "KnotPoint.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
}

namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( InterpolationMethod )
    FWD_DECLARE_SMART_PTRS( KnotPoints )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )

    /// UkpCurve stands for Unknowns as Knot-Point (y's) Curve
    /// i.e. a curve whose unknowns are some (usually most of)
    /// its knot-points' y's.
    class UkpCurve : public InterpolationCurve
    {
    public:
        UkpCurve();
        explicit UkpCurve(const KnotPointsPtr knotPoints,
                          const InterpolationMethodPtr interpolationMethod);

        static std::string getName()
        {
            return "UkpCurve";
        }

        static InterpolationCurvePtr createInstance(const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                    const KnotPointsPtr,
                                                    const LeastSquaresResidualsPtr leastSquaresResiduals);

        static InterpolationCurvePtr create(const LTQuant::GenericDataPtr interpolationCurveDetailsTable);

        virtual double interpolate(const double x) const;

        virtual void accumulateGradient(const double x, 
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd) const;

		// Obsolete: to be replaced with addKnotPointVariableToProblem
        // virtual void addUnknownsToProblem(const LTQuant::ProblemPtr& problem);
		virtual void addKnotPointVariableToProblem(const size_t kpIndex,
						 						   const LTQuant::ProblemPtr& problem);
		
        virtual void finalize();
        virtual void update();
        virtual size_t getNumberOfUnknowns() const;
		virtual double getUnknown(const size_t index) const;
		virtual void setUnknown(const size_t index, const double value);

        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        UkpCurve(UkpCurve const& original, CloneLookup& lookup);

    private:
        UkpCurve(UkpCurve const&); // deliberately disabled as won't clone properly

        InterpolationMethodPtr m_interpolationMethod;
    };  //  UkpCurve

    DECLARE_SMART_PTRS( UkpCurve )

}
#endif //__LIBRARY_PRICERS_FLEXYCF_UKPCURVE_H_INCLUDED