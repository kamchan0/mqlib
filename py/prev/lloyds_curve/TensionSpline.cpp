/*****************************************************************************
    
	Todo: - Add source file description

    @Originator		Nicolas Maury

    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved
*****************************************************************************/
#include "stdafx.h"
#include "TensionSpline.h"
#include "BSplineDefiningFunctions.h"
#include "ExponentialBSplineDefiningFunctions.h"
#include "SolverVariable.h"
#include "Maths\Problem.h"
#include "LeastSquaresResiduals.h"
#include "FixedKnotPointResidual.h"
#include "CurveAlgorithm.h"
#include "BSplineDefiningFunctionsFactory.h"
#include "Data\GenericData.h"
#include "KnotPoints.h"
#include "Math\QRdecomposition.h"
#include "Math\Matrix.h"
#include "Math\VectorDouble.h"
#include "FlexYCFCloneLookup.h"

using namespace std;
using namespace LTQC;

namespace FlexYCF
{
    string TensionSpline::getName()
    {
        return "TensionSpline";
    }

    // TO DO: - boundary conditions
    //        - initialization of the unknowns based on the knot-point values (linear inversion pb)
    //        - tension parameters initialization
    //        - index check-ups (seems ok now)
    //        - extrapolation / impact of the placement of the extra last three knots 
    TensionSpline::TensionSpline(const LeastSquaresResidualsPtr leastSquaresResiduals,
                                 const LTQuant::GenericDataPtr tensionParametersTable,
                                 const double defaultTension):
        m_leastSquaresResiduals(leastSquaresResiduals),
        m_defaultTension(defaultTension)
    {
        if(static_cast<bool>(tensionParametersTable))
        {
            int intervalIndex;  // corresponds to the left knot index, from 1 to M, extended to -2 to M + 3
            double tensionParameter;

            for(size_t k(0); k < tensionParametersTable->numItems() - 1; ++k)
            {
                intervalIndex    = tensionParametersTable->get<int>("Index", k);
                tensionParameter = tensionParametersTable->get<double>("Tension", k);     
            
                // At this point it is not known yet how many knots there is going to be.
                // So put the (index, tension) pair in the tension dictionary, if the index
                // is not already there.
                if(!m_tensionDictionary.exists(intervalIndex))
                {
                    m_tensionDictionary.insert(intervalIndex, tensionParameter);
                }
            }
        }
    }

    /**
        @brief A pseudo copy constructor with lookup.

        Uses a lookup to preserve directed graph relationships.

        @param original The instance to copy.
        @param lookup   A lookup of previously created clones.
    */
    TensionSpline::TensionSpline(TensionSpline const& original, CloneLookup& lookup):
        m_knotSequence(original.m_knotSequence),
        m_tensionParameters(original.m_tensionParameters),
        m_defaultTension(original.m_defaultTension),
        m_coefficients(original.m_coefficients),
        m_leastSquaresResiduals(lookup.get(original.m_leastSquaresResiduals)),
        m_tensionDictionary(original.m_tensionDictionary)
    {
        // Want to clone generation functions but use storage of this instance
        m_generatingFunctions = original.m_generatingFunctions->clone(m_knotSequence, m_tensionParameters);
    }
    
    /// Modifed static ctor
    InterpolationCurvePtr TensionSpline::createInstance(const LTQuant::GenericDataPtr interpolationCurveDetailsTable,
                                                           const KnotPointsPtr knotPoints,
                                                           const LeastSquaresResidualsPtr leastSquaresResiduals)
    {
		static const string defaultTensionSplineTypeName(ExponentialBSplineDefiningFunctions::getName());
        
        string tensionSplineTypeName(defaultTensionSplineTypeName);
        LTQuant::GenericDataPtr tensionParameters;
        double defaultTension(1.0);

        if(static_cast<bool>(interpolationCurveDetailsTable))
        {
            // Retrieve the name of the type of the spline defining functions family    
            interpolationCurveDetailsTable->permissive_get<string>("Interp", 0, tensionSplineTypeName, defaultTensionSplineTypeName);
            
            // Retrieve the tension parameters from a table
            interpolationCurveDetailsTable->permissive_get<LTQuant::GenericDataPtr>("Tension Parameters", 0, tensionParameters, LTQuant::GenericDataPtr());

            // Retrieve the default tension
            interpolationCurveDetailsTable->permissive_get<double>("Tension", 0, defaultTension, 1.0);
        }

        TensionSplinePtr tensionSpline(new TensionSpline(leastSquaresResiduals, tensionParameters, defaultTension));
    
        tensionSpline->m_generatingFunctions = BSplineDefiningFunctionsFactory::createBSplineDefiningFunctions(tensionSplineTypeName,
                                                                                                               tensionSpline->m_knotSequence,
                                                                                                               tensionSpline->m_tensionParameters);
        tensionSpline->setKnotPoints(knotPoints);

        return tensionSpline;
    }

    double TensionSpline::interpolate(const double x) const
    {
        const int index(getCoefIndex(x));    // must have:   1 <= index < # coefs
        // isn't index just equal to upper_bound(begin(), end(), x) - begin()??

        return getCoefficient(index-1) * getCoefWeight_1 (index, x) + getCoefficient( index ) * getCoefWeight(index, x)
            +  getCoefficient(index+1) * getCoefWeight_p1(index, x) + getCoefficient(index+2) * getCoefWeight_p2(index, x);
    }

    void TensionSpline::setGradient(int index, double multiplier, GradientIterator gradientBegin, double value) const
    {
        if(isUnknownKnotPoint(*(begin() + index)))
        {
            gradientBegin += count_if(begin(), begin() + index, isUnknownKnotPoint);
            *gradientBegin += multiplier * value;
        }
    }

    void TensionSpline::accumulateGradient(const double t, 
                                           double multiplier, 
                                           GradientIterator gradientBegin, 
                                           GradientIterator gradientEnd) const
    {
        const int nbOfCoefs(static_cast<int>(m_coefficients.size()));

        const int index(getCoefIndex(t)); 
        
        if(index > 1)
        {
            setGradient(index-2, multiplier, gradientBegin, getCoefWeight_1(index, t));
        }
        else
        {
            // hardcoded natural spline boundary condition g''(t[1]) = 0:
            const double yDiffRatio(computeYDiff(0) / computeYDiff(1));
            const double coefWeight_1(getCoefWeight_1(index, t));

            setGradient(0, multiplier, gradientBegin, (1.0 + yDiffRatio) * coefWeight_1);        // b[0] derivative relative to b[1]
            setGradient(1, multiplier, gradientBegin, -yDiffRatio * coefWeight_1);               // b[0] derivative relative to b[2]
        }

        setGradient(index-1, multiplier, gradientBegin, getCoefWeight(index, t));
        
        if(index < nbOfCoefs)
        {
            setGradient(index, multiplier, gradientBegin, getCoefWeight_p1(index, t));

            if(index < nbOfCoefs - 1)
            {
                setGradient(index+1, multiplier, gradientBegin, getCoefWeight_p2(index, t));
            }
            else 
            {
                // index == nbOfCoefs - 1
                const double yDiffRatio(computeYDiff(nbOfCoefs) / computeYDiff(nbOfCoefs-1));
                const double coefWeight_p2(getCoefWeight_p2(index, t));

                setGradient(nbOfCoefs-2, multiplier, gradientBegin, -yDiffRatio * coefWeight_p2);           // b[M+1] derivative relative to b[M-1]
                setGradient(nbOfCoefs-1, multiplier, gradientBegin, (1.0 + yDiffRatio) * coefWeight_p2);    // b[M+1] derivative relative to b[M]
            }
        }
        else if(index == nbOfCoefs)
        {
            // no b[M+2] coefficient,
            const double yDiffRatio(computeYDiff(nbOfCoefs) / computeYDiff(nbOfCoefs-1));
            const double coefWeight_p1(getCoefWeight_p1(index, t));

            setGradient(nbOfCoefs-2, multiplier, gradientBegin, -yDiffRatio * coefWeight_p1);           // b[M+1] derivative relative to b[M-1]
            setGradient(nbOfCoefs-1, multiplier, gradientBegin, (1.0 + yDiffRatio) * coefWeight_p1);    // b[M+1] derivative relative to b[M]
        }
        else
        {
            LT_THROW_ERROR( "Invalid Index " );
        }
    }

	/*
    void TensionSpline::addUnknownsToProblem(const LTQuant::ProblemPtr& problem)
    {
        // The unknowns are the coefficients in the functional representation
        for(vector<double>::iterator iter(m_coefficients.begin()); iter != m_coefficients.end(); ++iter)
        {
            problem->addVariable(SolverVariablePtr(new SolverVariable(*iter)));
        }
    }
	*/
	void TensionSpline::addKnotPointVariableToProblem(const size_t kpIndex,
													  const LTQuant::ProblemPtr& problem)
	{
		// There should be exactly the same number of knot-points and coefficients
		problem->addVariable(SolverVariablePtr(new SolverVariable(m_coefficients[kpIndex])));
	}
    
    void TensionSpline::finalize()
    {
        // Add 3 knots to the knot sequence before the beginning of the knot-points:
        const double firstKnot(begin()->x);
        
        // they have to be ordered, arbitrary placed for now
        m_knotSequence.push_back(firstKnot - 3.0);
        m_knotSequence.push_back(firstKnot - 2.0);
        m_knotSequence.push_back(firstKnot - 1.0);

        // add the knot-points' x's to the knot-point sequence
        //KnotPointContainer::const_iterator iter(begin());
        KnotPointConstIterator iter(begin());

        for(; iter != end(); ++iter)
        {
            m_knotSequence.push_back(iter->x);
        }

        // Add 3 knots to the sequence after the last point
        --iter;
        const double lastKnot(iter->x);

        // Again, arbitrary for now
        m_knotSequence.push_back(lastKnot + 1.0);
        m_knotSequence.push_back(lastKnot + 2.0);
        m_knotSequence.push_back(lastKnot + 3.0);


        // fill the tension parameters:
        int index = -2;
        for(size_t k(0); k < m_knotSequence.size(); ++k)
        {
            m_tensionParameters.push_back(m_tensionDictionary.exists(index) ?  m_tensionDictionary.get(index) : m_defaultTension);
            ++index;
        }
    }

     /* 2 problems:
        -   const_iterator's begin and end cannot be used directly  -> SOLVED with the intro of KnotPoints & modified design
        -   evaluate called on the last knot is not consistent with the fact
            that it is handled by left extrapolation at BaseCurve level
    */
    void TensionSpline::update()
    {
        //  Note: to ensure that f(x) = y for fixed knot-points (x, y), a "big" weight 
        //  can be set to corresponding extra residuals
		setYsFromXsWithFunction([this] (double x) {return interpolate(x);});
	}

	size_t TensionSpline::getNumberOfUnknowns() const
	{
		return m_coefficients.size();
	}

	double TensionSpline::getUnknown(const size_t index) const
	{
		return m_coefficients[index];
	}
	
	void TensionSpline::setUnknown(const size_t index, const double value)
	{
		m_coefficients[index] = value;
	}

    void TensionSpline::onKnotPointAdded(const KnotPoint& knotPoint)
    {
        // The addition of a new knot-point, known or not, induces
        // a new term, and hence a new coefficient, in the functional 
        // representation
        // Note: this can be moved to finalize (that's where tension 
        // parameters are added, depending on the # of knots). This should
        // simplify the synchronisation of the # of coefs for a tension 
        // spline in a composite curve. At the same time, there might be a
        // side effect with CompositeCurve where we need to know how many "real"
        // unknowns there are
        m_coefficients.push_back(1.0);

        // Need to test this: add FixedKnotPoint here and not at onKnotPointInitialized,
        //  keeping a reference to the KnotPoint y-value passed as a parameter
        if(knotPoint.isKnown && static_cast<bool>(m_leastSquaresResiduals))
        {
            // find the right index so as to pass evaluate and compute gradient ??
			m_leastSquaresResiduals->addExtraResidual(
				ExtraResidualPtr(
					new FixedKnotPointResidual(knotPoint.x,
						knotPoint.y,
						[this] (double x) {return interpolate(x);},
						[this] (double x, FlexYCF::Gradient& gradient) {calculateGradient(x, gradient);}
					)
				) 
			);
		}
    }
    
	// Not used anymore:
	/*
    void TensionSpline::onKnotPointInitialized(const KnotPoint& knotPoint)
    {    
        // Due to the fact that unknowns of the TensionSpline are not 
        // the knot-point values, each fixed (and initialized) knot-point
        // has to be mapped into an equivalent condition
        if(knotPoint.isKnown)
        {
            // find the right index so as to pass evaluate and compute gradient ??
            m_leastSquaresResiduals->addExtraResidual(
                ExtraResidualPtr(
                    new FixedKnotPointResidual(
						knotPoint.x,
                        knotPoint.y,
                        std::tr1::bind(&TensionSpline::interpolate, this, _1),
                        std::tr1::bind(&TensionSpline::calculateGradient, this, _1, _2)
                    )
                )
			);
        }
    }
	*/
    
    void TensionSpline::calculateGradient(const double x, Gradient& gradient) const
    {
        gradient.clear();
        gradient.resize(getNumberOfUnknowns());
        accumulateGradient(x, 1.0, gradient.begin(), gradient.end());
    }

    void TensionSpline::setBSplineDefiningFunctions(const BSplineDefiningFunctionsPtr generatingFunctions)
    {
        m_generatingFunctions = generatingFunctions;
    }

	// Not fully tested: first results don't seem too bad
	void TensionSpline::onKnotPointsInitialized()
	{	
		// solve the system A * coefs = y, where:
		//	A is a n x n matrix, with 0 everywhere but on a four element-wide diagonal
		//	coefs is a n-dim vector that contains the coefficients of the tension spline to initialize
		//	mod_y is a n-dim vector that contains the y-values of the knot-points, modified for knots 1, n-1 and n
		LT_LOG << " - Initialization of " << m_coefficients.size() << " Tension Spline coefficients -!" << endl;

		// 1. Resize A, coefs and mod_y:
		const size_t size(m_coefficients.size());
		LTQC::Matrix A(size, size, 0.);
		LTQC::VectorDouble coefs(size), mod_y(size);
		
		LT_LOG << "A: " << std::endl;
		
		// 2. Fill A and mod_y
		// A should be mostly 0, except on a 4-elements wide diagonal where it has "getCoefWeight" values
		// except for boundaries because 'coef(0)' and 'coef(M+1)' are set by the boundary conditions
		size_t colMax;
		double x;
		int index;
		for(size_t row(0); row < size; ++row)
		{
			colMax = min(size, row + 3);	// so that row - 1 <= col <= row + 2 in the loop below
			index = static_cast<int>(row+1);
			x = getKnotFromIndex(index);
			
			for(size_t col(max(0, static_cast<int>(row) - 1)); col < colMax; ++col)
			{	
				if(col == row - 1)
					A(row,col) = getCoefWeight_1(index, x);
				else if (col == row)
					A(row,col) = getCoefWeight(index, x);
				else if(col == row + 1)
					A(row,col) = getCoefWeight_p1(index, x);
				else if(col == row + 2)
					A(row,col) = getCoefWeight_p2(index, x);
				else
					throw std::exception("invalid subscript while initializing the coefficients of TensionSpline");
			
				LT_LOG << A(row,col) << "\t";
			}
			
			// init the modified y's to the knot-point y's
			mod_y[row] = getKnotPoint(row).y;
			
			LT_LOG << std::endl;
		}

		// modify 'y(1)', 'y(M-1)' and 'y(M)':
		mod_y[0] -= getCoefficient(0) * getCoefWeight_1(1, getKnotFromIndex(1));
		if(size > 1)
		{
			index = static_cast<int>(size);
			mod_y[size-1] -= getCoefficient(index+1) * getCoefWeight_p1(index, getKnotFromIndex(index));
			if(size > 2)
				mod_y[size-2] -= getCoefficient(index+1) * getCoefWeight_p2(index-1, getKnotFromIndex(index-1));
		}

		// 3. solve using QR decomposition
		qr_solve(A, mod_y, coefs);
		

		// 4. initialize the unknown coefs of the tension spline
		for(size_t k(0); k < size; ++k)
			m_coefficients[k] = coefs[k];

		// 5. Check the coefs found are correct, i.e. interpolate(x) = y for all kp (x,y) 
		LT_LOG << "mod y\ty\tf(x)\tcoef" << endl;
		for(int k(0); k < static_cast<int>(size); ++k)
		{
			LT_LOG <<  mod_y[k] << "\t" << getKnotPoint(k).y << "\t" << interpolate(getKnotPoint(k).x) << "\t" << m_coefficients[k] << endl;
		}
	}

    /**
        @brief Clone this instance.

        Uses a lookup to ensure directed graph relationships are maintained.

        @param lookup The lookup of previously created clones.
        @return       A clone of this instance.
    */
    ICloneLookupPtr TensionSpline::cloneWithLookup(CloneLookup& lookup) const
    {
        return ICloneLookupPtr(new TensionSpline(*this, lookup));
    }

    std::ostream& TensionSpline::print(std::ostream& out) const
    {
        // TO DO
        out << "TensionSpline";
        return out;
    }

    /// Returns the index k of the knot-points such that t[k] <= t < t[k+1],
    /// i.e. sup{ 1<=k<M : t >= t[k] }, where M is the 
    /// number of original knot-points indexed from 1 to M
    int TensionSpline::getCoefIndex(const double t) const
    {
        //const KnotPointContainer::const_iterator lastKnotPoint(end() - 1);
        const KnotPointConstIterator lastKnotPoint(end() - 1);
        if(t >= lastKnotPoint->x)
        {
            return static_cast<int>(m_coefficients.size()); // does not take into account the first three extra knots
        }

        size_t index;
        for(index = 2; index < m_knotSequence.size() && t >= m_knotSequence[index]; ++index)
            ;

        return static_cast<int>(index) - 3;
    }
 
    /// new function
    int TensionSpline::getCoefIndex(const KnotPointConstIterator upperBound) const
    {
        return static_cast<int>(upperBound - begin());
    }
        

    /// Returns the value of the index-th knot, starting from 1
    ///  and ignoring the knots added at the beginning for 
    ///  boundary condition
    double TensionSpline::getKnotFromIndex(const int index) const
    {
        return m_knotSequence[index + 2];
    }

    double TensionSpline::computeZ(const int index) const
    {
        double tmp(m_generatingFunctions->psi(index - 1, getKnotFromIndex(index)) - m_generatingFunctions->phi(index, getKnotFromIndex(index)));
        return tmp;
    }

    double TensionSpline::computeZDerivative(const int index) const
    {
        double tmp(m_generatingFunctions->psiDerivative(index - 1, getKnotFromIndex(index)) - m_generatingFunctions->phiDerivative(index, getKnotFromIndex(index)));
        return tmp;
    }

    double TensionSpline::computeY(const int index) const
    {
        double tmp(getKnotFromIndex(index) - computeZ(index) / computeZDerivative(index));
        return tmp;
    }

    ///  YDiff[k] = y[k+1] - y[k]
    double TensionSpline::computeYDiff(const int index) const
    {
        double tmp(computeY(index+1) - computeY(index));
        return tmp;
    }

    double  TensionSpline::computePhiOnZDer(const int index, const double t) const
    {
        double tmp(m_generatingFunctions->phi(index, t) / computeZDerivative(index));
        return tmp;
    }
    
    double  TensionSpline::computePsiOnZDer(const int index, const double t) const
    {
        double tmp(m_generatingFunctions->psi(index, t) / computeZDerivative(index+1));
        return tmp;
    }

    // a value common to central coefs
    double TensionSpline::computePhiPsiCoef(const int index, const double t) const
    {
        double tmp((t - computeY(index) + computePhiOnZDer(index, t) - computePsiOnZDer(index, t)) / computeYDiff(index));
        return tmp;
    }

    double TensionSpline::getCoefWeight_1(const int index, const double t) const
    {
        double tmp(computePhiOnZDer(index, t) /  (computeY(index) - computeY(index-1)));
        return tmp;
    }

    double TensionSpline::getCoefWeight(const int index, const double t) const
    {
        double tmp(1.0 - computePhiPsiCoef(index, t) - computePhiOnZDer(index, t) / computeYDiff(index-1));
        return tmp;
    }
    
    double TensionSpline::getCoefWeight_p1(const int index, const double t) const
    {
        double tmp(computePhiPsiCoef(index, t) - computePsiOnZDer(index, t) / computeYDiff(index+1));
        return tmp;
    }
    
    // TO DO: if index = # coefs - 1, so that we must return the M+1 coef gradient,
    // return the corresponding boundary condition gradient
    double TensionSpline::getCoefWeight_p2(const int index, const double t) const
    {
        double tmp(computePsiOnZDer(index, t) / computeYDiff(index+1));
        return tmp;
    }

    double TensionSpline::getCoefficient(const int index) const
    {
        const int nbOfCoefs(static_cast<int>(m_coefficients.size()));   // TO DO: put the int nbOfCoefs in finalize
        if(0 < index && index <= nbOfCoefs)
        {
            return m_coefficients[index - 1];
        }

        // hardcoded natural spline boundary conditions (i.e. g''(t[1]) = g''(t[M]) = 0 ) for now:
        if(index == 0)
        { 
            return m_coefficients[0] - (m_coefficients[1] - m_coefficients[0]) * computeYDiff(0) / computeYDiff(1);
        }
        
        if(index == nbOfCoefs + 1)
        {
            const int lastIndex(nbOfCoefs - 1);
        
            return m_coefficients[lastIndex] 
                + (m_coefficients[lastIndex] - m_coefficients[lastIndex-1]) * computeYDiff(lastIndex+1) / computeYDiff(lastIndex);
        }

        // index < 0 || index > nbOfCoefs + 1
        return 0.0; // or throw error, or at least log info
    }

}