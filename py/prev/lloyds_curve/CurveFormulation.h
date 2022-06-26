/*****************************************************************************

	CurveFormulation

	Models a financial curve in a certain domain space


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CURVEFORMULATION_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CURVEFORMULATION_H_INCLUDED
#pragma once

//	FlexYCF
#include "KnotPoint.h"
#include "KnotPoints.h"
#include "Gradient.h"
#include "ICloneLookup.h"

//	LTQC
#include "VectorDouble.h"


namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
    FWD_DECLARE_SMART_PTRS( Problem )
}

namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BaseCurve )
    FWD_DECLARE_SMART_PTRS( CurveFormulation )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals)

	class IKnotPointFunctor;

	typedef std::vector<std::vector<std::pair<double, double>>> knot_points_container;

    /// An abstract class encapsulating the concept of a
    /// financial curve formulation.
    class CurveFormulation : public ICloneLookup
    {
    protected:
        explicit CurveFormulation(const LTQuant::GenericDataPtr& interpolationDetailsTable,
                                  const std::string& curveDescription,
                                  const LeastSquaresResidualsPtr& leastSquaresResiduals);
        CurveFormulation(CurveFormulation const& original, CloneLookup& lookup);
        virtual ~CurveFormulation() = 0 { }
    
    public:
        virtual double getDiscountFactor(const double flowTime) const = 0;
        virtual void accumulateDiscountFactorGradient(const double x, 
													  double multiplier, GradientIterator gradientBegin, GradientIterator gradientEnd) const = 0;
		virtual double getVariableValueFromSpineDiscountFactor(const double flowTime,
														  const double discountFactor) const = 0;

        // the transform from the curve formulation space 
        //  to the discount factor space
        //virtual double transform(const double x) const;         // TO DO
        //virtual double inverseTransform(const double x) const;  // TO DO

        std::string getCurveDescription() const { return m_curveDescription; }

        // The following functions are just delegated to the inner curve
        
        // for now knot-points are assumed to be added in the correct space
        void addKnotPoint(const KnotPoint& knotPoint) const;
        void addKnotPoint(const double x, const double y) const;  
        void addFixedKnotPoint(const double x, const double y) const;
        virtual void setSpotRate(double spotTime, double spotRate);
        // which is not ideal... to be replaced by:
        // void addInstantaneousForward
        
        void addUnknownsToProblem(const LTQuant::ProblemPtr problem) const; 
		void addUnknownsToProblem(const LTQuant::ProblemPtr problem,
								  IKnotPointFunctor& onKnotPointVariableAddedToProblem) const; 
		void updateVariablesFromShifts(const LTQC::VectorDouble& variableShifts);
		void finalize() const;
        void update() const; 

        size_t getNumberOfUnknowns() const;

        // void initializeWithSpotRate(const double spotRate) const;
        
        // Initializes the knot-points
        virtual void initializeKnotPoints() const = 0;
		
		void onKnotPointsInitialized() const;

        double getInitSpotRate() const;
    
		LTQuant::GenericDataPtr getSpineCurveDetails() const;

		void getCurveInternalData(knot_points_container& kpc) const;

		void assignCurveInternalData(knot_points_container::const_iterator it);

        void getSpineCurvesUnfixedKnotPoints(std::list<double>& points) const;
		 
		KnotPoints::const_iterator begin() const;
		KnotPoints::const_iterator end() const;
		std::vector<double> abscissas() const;
    protected:
        void enforceFixedKnotPoint(const double x, const double y) const;
       
        // Put here as it is common to DiscountCurve and InstantaneousForwardCurve
        double initSpotRateAsFunction(const double) const
        {
            return m_initSpotRate;
        }

        BaseCurvePtr m_baseCurve;
    
    private:
		friend void setSeparationPoint(const LT::date,const CurveFormulation&);
        virtual void onFinalize() const;
        CurveFormulation(CurveFormulation const&); // deliberately disabled as won't clone properly

        std::string m_curveDescription;
        double m_initSpotRate;
    };  // CurveFormulation
}
#endif // __LIBRARY_PRICERS_FLEXYCF_CURVEFORMULATION_H_INCLUDED