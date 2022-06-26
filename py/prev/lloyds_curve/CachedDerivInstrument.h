/*****************************************************************************
	CachedDerivInstrument

	Represents a curve instrument with PV, BPV and RateDerivative cached
    so they can be used in the jacobian transformations later


    @Originator		Mark Ayzenshteyn

    Copyright (C) Lloyds TSB Group plc 2011 All Rights Reserved
*****************************************************************************/
#ifndef __CachedDerivInstrument_H__
#define __CachedDerivInstrument_H__


#include "RiskInstrument.h"

namespace FlexYCF
{
    class CachedDerivInstrument: public RiskInstrument
    {
    public:
        CachedDerivInstrument(const double rate,
            const LT::Str& name,
            const LT::Str& description):RiskInstrument(rate,name,description){}
        //these will be implemented by Calibration instrument, but users of this class need to know if this instr was placed or not
        virtual inline bool wasPlaced() const=0;
        //and also know the type
        virtual std::string getType() const = 0;

        double getBPV() const;
        double getRateDerivative() const;

    protected:
        CachedDerivInstrument():RiskInstrument(){ }
        void setBPV(const double& d);
        void setRateDerivative(const double& d);

    private:
        enum CachedTypes 
        {
            BPV=0,
            RateDeriv,
            CachedTypesSize
        };

        struct CheckedDouble
        {
            CheckedDouble():isPopulated(false){}
            double operator()() const
            {
                if(!isPopulated)
                {
                    LTQC_THROW( LTQC::MathQCException, "Cannot access uninitialised value" );
                }
                else
                {
                    return value;
                }
            }
            void operator() (const double &d)
            {
                value=d; 
                isPopulated=true;
            }
        private:
            bool isPopulated;
            double value;
        };
       CheckedDouble m_cachedValues[CachedTypesSize];
    };

     DECLARE_SMART_PTRS( CachedDerivInstrument )

    namespace
    {
        template<class T>
        bool is_of_type(const CachedDerivInstrument& instrument)
        {
            return (instrument.getType() == T::getName());
        }

        template<class T>
        bool isOfType(const CachedDerivInstrumentPtr& instrument)
        {
            return is_of_type<T>(*instrument);
        }
    }
   
}

#endif