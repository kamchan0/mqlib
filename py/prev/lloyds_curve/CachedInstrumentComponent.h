/*****************************************************************************

    Todo: - Add header file description
    
    @Originator
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_CACHEDINSTRUMENTCOMPONENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_CACHEDINSTRUMENTCOMPONENT_H_INCLUDED
#pragma once

#include "InstrumentComponent.h"
#include "GenericInstrumentComponent.h"


namespace FlexYCF
{
    /// CachedInstrumentComponent is a template class that,
    /// given an Arguments class, creates a cache for the
    /// corresponding templatized instrument component type.
    template <class Arguments,
              class TComponent = GenericInstrumentComponent<Arguments> >
    class CachedInstrumentComponent : public TComponent
    {
    private:
        DECLARE_SMART_PTRS( TComponent )

        CachedInstrumentComponent(const Arguments& arguments):
            TComponent(arguments),
            m_isValueComputed(false),
            m_isGradientComputed(false)
        {
        }

        /**
            @brief Pseudo copy constructor.

            This copy constructor is used in cloning where directed graph relationships must be maintained.

            @param original The instance to copy.
            @param lookup   A lookup of previously created clones.
        */
        CachedInstrumentComponent(CachedInstrumentComponent const& original, CloneLookup& lookup) :
            TComponent(original, lookup),
            m_isValueComputed(false),
            m_isGradientComputed(false)
        {
        }

    public:
        static  
        TComponentPtr create(const Arguments& arguments)
        {
            return TComponentPtr(new CachedInstrumentComponent(arguments));
        }

        /// Returns the value of the cached instrument component
        /// in the specified model
        virtual double getValue(BaseModel const& baseModel)
        {
            if(!m_isValueComputed)
            {
                m_value = TComponent::getValue(baseModel);
                m_isValueComputed = true;
            }
            return m_value;
        }

        /// Computes the value of the cached instrument component
        /// in the specified model
        virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier,
                                        GradientIterator gradientBegin,
                                        GradientIterator gradientEnd)
        {
            size_t const length = std::distance(gradientBegin, gradientEnd);
            if(!m_isGradientComputed)
            {
                m_gradient.assign(length, 0.0);
                TComponent::accumulateGradient(baseModel, 1.0, m_gradient.begin(), m_gradient.end());
                m_isGradientComputed = true;
            }
            double* it1 = &(*gradientBegin);
            double* it2 = &m_gradient.front();
            for(size_t i = 0; i < length; ++i)
            {
                (*it1) += multiplier * (*it2);
                ++it1;
                ++it2;
            }
        }

        /// Updates the cached instrument component
        inline virtual void update()
        {
            clear();
            TComponent::update();
        }
        
        /// Clears the instrument component
        void clear()
        {
            m_isValueComputed = false;
            m_isGradientComputed = false;
        }

        /**
            @brief Create a clone of this instance.

            Ensure that the base class is copied using a lookup so directed graph relationships are maintained.

            @param lookup A lookup of previously created clones.
        */
        ICloneLookupPtr cloneWithLookup(CloneLookup& lookup) const
        {
            return ICloneLookupPtr(new CachedInstrumentComponent(*this, lookup));
        }

    private:
        CachedInstrumentComponent(CachedInstrumentComponent const&); // deliberately disabled as won't clone properly

        bool m_isValueComputed;
        double m_value;
        bool m_isGradientComputed;
        std::vector<double> m_gradient;
    };  //  CachedInstrumentComponent

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_CACHEDINSTRUMENTCOMPONENT_H_INCLUDED