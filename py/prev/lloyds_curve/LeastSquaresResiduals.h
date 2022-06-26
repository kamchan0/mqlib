/*****************************************************************************

	LeastSquaresResiduals
    
	Represents the residuals in the least squares problem to solve during
	calibration of the model.


    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESRESIDUALS_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESRESIDUALS_H_INCLUDED
#pragma once

//	FlexYCF
#include "InstrumentResiduals.h"
#include "ExtraResiduals.h"
#include "CalibrationInstruments.h"
#include "Gradient.h"
#include "ICloneLookup.h"
#include "ResidualsUtils.h"


namespace LTQuant
{
	FWD_DECLARE_SMART_PTRS( LeastSquaresProblem )
}


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( BaseModel )
    FWD_DECLARE_SMART_PTRS( CalibrationInstrument )
    FWD_DECLARE_SMART_PTRS( ExtraResidual )
	FWD_DECLARE_SMART_PTRS( CurveType )

    /// Represents the collection of all residuals
    /// of a weighted least squares problem related 
    /// to a BaseModel.
    /// Residuals inside LeastSquaresResiduals either
    /// belong to InstrumentResiduals if they are related
    /// to the fitting of an instrument model price to
    /// its market price. Otherwise they belong to ExtraResiduals.
    class LeastSquaresResiduals : public ICloneLookup
    {
    public:
        explicit LeastSquaresResiduals(const BaseModelPtr baseModel);
		explicit LeastSquaresResiduals(BaseModel* const model);

		inline void setInstrumentResidualRepresentationType(const LeastSquaresRepresentationType::Enum_t instrumentResidualsRepresentationType)
		{
			m_instrumentResiduals.setResidualRepresentationType(instrumentResidualsRepresentationType);
		}
        void addInstrumentResidual(const CalibrationInstrumentPtr instrument,
                                   const double weight = 1.0);
       
        // Add all calibration instruments - use only once the instruments have been "selected"
        //  i.e. the ones not used in calibration have been removed
        void addInstrumentResiduals(const CalibrationInstruments& instruments,
                                    const double weight = 1.0);

        /// Add an extra residual to the instance.
        void addExtraResidual(const ExtraResidualPtr extraResidual);
        
        /// Returns the number of residuals.
        size_t size() const;

		size_t sizeOfInstrumentResidual() const;

		/// Returns the index-th residual
		WeightedResidualPtr operator[](const size_t index) const;

		// Return the index-th instrument residual
		InstrumentResidualPtr getInstrumentResidualAt(const size_t index) const;

        // setInstrumentResidualWeight(const size_t index, const double weight);
        // setExtraResidualWeight(const size_t index, const double weight);
        // setExtraResidualsWeight(const double weight);
        
        /// Returns the value of the index-th weighted residual.
        double evaluate(const size_t index) const;

        /// Computes the gradient (relative the unknowns 
        /// of the problem of the index-th weighted residual. 
        void computeGradient(const size_t index, Gradient& gradient) const;

		/// Computes the gradient (relative the unknowns 
        /// of the problem of the index-th weighted residual
		/// relative to the variables of the specified curve type
		void computeGradient(const size_t index, Gradient& gradient, const CurveTypeConstPtr& curveType) const;

        /// Delegates the update to the model its holds
        /// and all its residuals, in this order.
        void update();

		/// Create a least squares problem corresponding to the residuals
		LTQuant::LeastSquaresProblemPtr createLeastSquaresProblem();// const;

		/// Clears all the residuals 
		void clear();

        //after calib is finished destory all of the calibration
        //instruments so they do not take up memory via the CachedInstrument component
        void finishCalibration();
        /// Prints the LeastSquaresResiduals
        std::ostream& print(std::ostream& out) const;

        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const;

    protected:
        LeastSquaresResiduals(LeastSquaresResiduals const& original, CloneLookup& lookup);

    private:
        void checkIndex(const size_t index) const;
        LeastSquaresResiduals(LeastSquaresResiduals const&); // deliberately disabled as won't clone properly

        BaseModelPtr m_baseModel;   // has to be here to update the model in update()

        InstrumentResiduals m_instrumentResiduals;
        ExtraResiduals		m_extraResiduals;
    };  //  LeastSquaresResiduals

    DECLARE_SMART_PTRS( LeastSquaresResiduals )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const LeastSquaresResiduals& leastSquaresResiduals)
		{
			return leastSquaresResiduals.print(out);
		}
	}
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_LEASTSQUARESRESIDUALS_H_INCLUDED