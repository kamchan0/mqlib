/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_GENERICINSTRUMENTCOMPONENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_GENERICINSTRUMENTCOMPONENT_H_INCLUDED
#pragma once

#include "InstrumentComponent.h"
#include "Gradient.h"
#include "ICloneLookup.h"

namespace FlexYCF
{

    /// GenericInstrumentComponent is template class that 
    /// can be used to create concrete instrument components,
    /// given the type of their arguments.
    template <class Arguments>
    class GenericInstrumentComponent : public InstrumentComponent,
                                       public ICloneLookup
    {
    private:
        typedef GenericInstrumentComponent<Arguments>   Component;
        DECLARE_SMART_PTRS( Component )
    
    protected:
        GenericInstrumentComponent(const Arguments& arguments) :
            m_arguments(arguments)
        { }

    public:
        typedef Arguments   Arguments;

        /// Creates a smart pointer to a GenericInstrumentComponent<Arguments>
        static ComponentPtr create(const Arguments& arguments)
        {
            return ComponentPtr(new GenericInstrumentComponent(arguments));
        }

        virtual double getValue(BaseModel const& baseModel);
        virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd);
		 virtual void accumulateGradient(BaseModel const& baseModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType);
		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										bool spread);
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd,
										bool spread);
        virtual void update();

        const Arguments& getArguments() const
        {
            return m_arguments;
        }

        Arguments& getArguments()
        {
            return m_arguments;
        }

        bool operator==(const GenericInstrumentComponent<Arguments>& other) const
        {
            return m_arguments == other.m_arguments;
        }
        
        virtual std::ostream& print(std::ostream& out) const
        {
            m_arguments.print(out);
            return out;
        }

        /**
            @brief Create a clone of this instance.

            This assumes that the instance will does not contain any other instances that need cloning.

            @param lookup This lookup is ignored here.
            @return       Return a clone of this instance.
        */
        virtual ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const
        {
            return ICloneLookupPtr(new GenericInstrumentComponent<Arguments>(*this, lookup));
        }

    protected:
        /**
            @brief Pseudo copy constructor.

            This assumes the instance does not contain any other instances that need cloning.

            @param original The instance to clone from.
        */
        GenericInstrumentComponent(GenericInstrumentComponent<Arguments> const& original, CloneLookup&) :
            m_arguments(original.m_arguments)
        {
        }

    private:
        GenericInstrumentComponent(GenericInstrumentComponent const&);  // deliberately disabled as won't clone properly

        Arguments m_arguments;
    };  //  GenericInstrumentComponent

	template<class Arguments>
    size_t hash_value(const GenericInstrumentComponent<Arguments>& instrumentComponent)
    {
        return hash_value(instrumentComponent.getArguments());
    }
}
#endif //__LIBRARY_PRICERS_FLEXYCF_GENERICINSTRUMENTCOMPONENT_H_INCLUDED