/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_COMPOSITECURVE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_COMPOSITECURVE_H_INCLUDED
#pragma once

#include "InterpolationCurve.h"
#include "KnotPoint.h"
#include "KnotPoints.h"

// IDeA
// #include "UtilsEnums.h"

#include "src/Enums/CompositeSeparationType.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS;
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( KnotPoints )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )

    /// Represents the composition of a left and a right
    /// InterpolationCurve's
    class CompositeCurve : public InterpolationCurve
    {
    public:
        static std::string getName();

        static InterpolationCurvePtr createInstance(const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                    const KnotPointsPtr knotPoints,
                                                    const LeastSquaresResidualsPtr leastSquaresResiduals);

		//	Sets the separation x
		void inline setSeparationX(const double separationX)	
		{	
			m_separationX		= separationX;	
			m_separationType	= IDeA::CompositeSeparationType::Assigned;
		}

		inline IDeA::CompositeSeparationType getSeparationType() const						
		{	
			return m_separationType; 
		}

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

        virtual void onKnotPointAdded(const KnotPoint& knotPoint);
    
        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        CompositeCurve(CompositeCurve const& original, CloneLookup& lookup);

    private:
        InterpolationCurvePtr		m_leftCurve,
									m_rightCurve;
        double						m_separationX;
        LeastSquaresResidualsPtr	m_leastSquaresResiduals;

		IDeA::CompositeSeparationType m_separationType;

        CompositeCurve();
        CompositeCurve(CompositeCurve const&); // deliberately disabled as won't clone properly
    };

    DECLARE_SMART_PTRS( CompositeCurve );
}

#endif //__LIBRARY_PRICERS_FLEXYCF_COMPOSITECURVE_H_INCLUDED