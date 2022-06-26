/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_MULTITENORDEFAULTKPP_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_MULTITENORDEFAULTKPP_H_INCLUDED
#pragma once

#include "MultiCurveKpp.h"
#include "SparseSetGrid.h"
#include "CurveType.h"


namespace FlexYCF
{
    FWD_DECLARE_SMART_PTRS( MultiTenorModel )
    FWD_DECLARE_SMART_PTRS( CalibrationInstrument )

    /// MultiTenorDefaultKpp is the default knot-point placement 
    /// algorithm for the multi-tenor model.
    class MultiTenorDefaultKpp : public MultiCurveKpp
    {
    public:
        static std::string getName()
        {
            return "MultiTenorKpp0";
        }

        static MultiCurveKppPtr createInstance(const LTQuant::GenericDataPtr)
        {
            return MultiCurveKppPtr(new MultiTenorDefaultKpp);
        }

        virtual bool createKnotPoints(const CalibrationInstruments& instruments, 
                                      const BaseModelPtr baseModel);

        typedef SparseSetGrid< CurveTypeConstPtr,
                               LT::date,
                               CalibrationInstrumentConstPtr,
                               CurveTypeCompare
                             > InstrumentSetGrid;

    protected:
        InstrumentSetGrid m_instrumentSetGrid;
        
        virtual void fillInstrumentSetGrid(const CalibrationInstruments& instruments);
        virtual void addKnotPointsToBaseRateCurve(const MultiTenorModelPtr multiTenorModelPtr);         // default implementations provided...
        virtual void addKnotPointsToDiscountSpreadCurve(const MultiTenorModelPtr multiTenorModelPtr);   // .. but those can be overriden in
        virtual void addKnotPointsToSpreadSurface(const MultiTenorModelPtr multiTenorModelPtr);         // derived classes if wanted

        virtual void removeInstrumentsFromSetGridCells();
        virtual std::vector<CurveTypeConstPtr> determineCurveTypes(const std::vector<CurveTypeConstPtr>& curveTypes) const;

    private:

        size_t m_numberOfInputVariables;
        CurveTypeConstPtr m_baseRate;
        
    };  //  MultiTenorDefaultKpp

    DECLARE_SMART_PTRS( MultiTenorDefaultKpp )
  
}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_MULTITENORDEFAULTKPP_H_INCLUDED