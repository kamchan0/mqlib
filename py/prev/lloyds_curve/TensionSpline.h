/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_NEWTENSIONSPLINE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_NEWTENSIONSPLINE_H_INCLUDED
#pragma once

#include "InterpolationCurve.h"
#include "Dictionary.h"

namespace LTQuant
{
    FWD_DECLARE_SMART_GENERIC_DATA_PTRS
    FWD_DECLARE_SMART_PTRS( Problem )
}

namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( BSplineDefiningFunctions )
    FWD_DECLARE_SMART_PTRS( LeastSquaresResiduals )
    FWD_DECLARE_SMART_PTRS( KnotPoints )

    /// Represents generalized tension spline
    ///
    /// Note: unlike UkpCurve, the unknowns of a tension spline
    /// are its coefficients that weigh the basis functions
    /// sum b[k] * B_k(t),  b_k are the unknowns
    /// The implementation closely follows "Discount Curve Construction
    /// with Tension Splines", Leif Andersen.
    /// When all "real" knots have been placed, calling finalize adds three
    /// additional knots before the first knot and three additional knots
    /// after the last knot: if we assume the original knots are {t[i]; 1 <= i <= M}
    /// the enlarged knot-sequence will be {t[i]; -2 <= i <= M+3} 
    class TensionSpline : public InterpolationCurve 
    {
    protected:
        explicit TensionSpline(const LeastSquaresResidualsPtr leastSquaresResiduals,
                                  const LTQuant::GenericDataPtr tensionParameters,
                                  const double defaultTension);
        TensionSpline(TensionSpline const& original, CloneLookup& lookup);

    public:
        static std::string getName();

        static InterpolationCurvePtr createInstance(const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                    const KnotPointsPtr knotPoints,
                                                    const LeastSquaresResidualsPtr leastSquaresResiduals);

        /// From InterpolationCurve interface:
        virtual double interpolate(const double x) const;

        virtual void accumulateGradient(const double x,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd) const; 
        
        virtual void onKnotPointAdded(const KnotPoint& knotPoint);

		// Obsolete: to be replaced with addKnotPointVariableToProblem
        // virtual void addUnknownsToProblem(const LTQuant::ProblemPtr& problem); 
        virtual void addKnotPointVariableToProblem(const size_t kpIndex,
												   const LTQuant::ProblemPtr& problem);
		
		virtual void finalize();
        virtual void update(); 
        virtual size_t getNumberOfUnknowns() const;
		virtual double getUnknown(const size_t index) const;
		virtual void setUnknown(const size_t index, const double value);
        
		/// Sets the defining functions that specifies the tension spline.
        void setBSplineDefiningFunctions(const BSplineDefiningFunctionsPtr generatingFunctions);

		/// Calculates the coefficients that match the y's of the knot-points
		virtual void onKnotPointsInitialized();

        virtual std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

     private:
        // virtual void onKnotPointInitialized(const KnotPoint& knotPoint);
        void calculateGradient(const double x, Gradient& gradient) const; 
   
        int getCoefIndex(const double t) const;

        int getCoefIndex(const KnotPointConstIterator upperBound) const;
        void setGradient(int index, double multiplier, GradientIterator gradientBegin, double value) const;
        
        inline 
        double getKnotFromIndex(const int index) const; 

        double computeZ(const int index) const;
        double computeZDerivative(const int index) const;
        double computeY(const int index) const;
        inline
        double computeYDiff(const int index) const;     

        double computePhiOnZDer(const int index, const double t) const;
        double computePsiOnZDer(const int index, const double t) const;
        double computePhiPsiCoef(const int index, const double t) const; 

        double getCoefWeight_1(const int index, const double t) const;
        double getCoefWeight(const int index, const double t) const;
        double getCoefWeight_p1(const int index, const double t) const;
        double getCoefWeight_p2(const int index, const double t) const;
        double getCoefficient(const int index) const;

        
        BSplineDefiningFunctionsPtr m_generatingFunctions;

        // that concrete derived function would determine
        // they also depend on the knot-points and on the
        // tension parameter (can be the same for all intervals
        //  or interval specific).

        /// Contains the ordered sequence of the x's of the knot-points,
        /// plus the additional knots before and after those (6 in total)
        /// m_knotSequence[k] corresponds to knot t[k-2], 0 <= k <= M+5
        std::vector<double> m_knotSequence;

        /// Contains tension parameters on each interval [t[k], t[k+1])
        /// m_tensionParameters[k] corresponds to the tension parameter
        /// on interval [t[k-2], t[k-1]),  0 <= k <= M+4
        std::vector<double> m_tensionParameters;     
        
        /// Default tension parameter that applies to an interval
        /// for which to explicit tension parameter is given
        const double m_defaultTension;

        // coefficients b[k] s.t. f(t) = sum b[k] * B[k](t)
        // if M = # knots, and the b's are ordered from 1 to M,
        //  b[0] and b[M+1] can be expressed in terms of the others
        //  depending on the chosen boundary conditions and are not
        // stored in this std::vector
        std::vector<double> m_coefficients;

        const LeastSquaresResidualsPtr m_leastSquaresResiduals;

        /// A collection of (index, tension parameters) pairs to 
        /// simplify the loading of tension parameters from a table
        Dictionary<int, double> m_tensionDictionary;    

    private:
        TensionSpline(TensionSpline const&); // deliberately disabled as won't clone properly
    };

    DECLARE_SMART_PTRS( TensionSpline ) 

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_NEWTENSIONSPLINE_H_INCLUDED