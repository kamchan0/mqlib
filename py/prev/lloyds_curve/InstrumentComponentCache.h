/*****************************************************************************

    Todo: - Add header file description
    
    @Originator	Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/
#ifndef __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOMPONENTCACHE_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOMPONENTCACHE_H_INCLUDED
#pragma once

#include "Cache.h"
#include "InstrumentComponent.h"
#include "CachedInstrumentComponent.h"
#include "GenericInstrumentComponent.h"


namespace FlexYCF
{
    /// InstrumentComponentCache is a factory that stores and cache the (shared 
    /// pointers to) the concrete instrument components created and avoids 
    /// creating the same component twice by checking the Arguments specified
    /// to build the component is different to those that are already stored
    /// internally. 
    ///
    /// Implementation Note: it uses an internal cache which compares the 
    /// arguments to create the object. A arguments class must exist for the
    /// creation of this kind of classes and it must contain an inner functor
    /// 'Compare' that order the arguments (however, this is not necessary if
    /// less<Arguments> makes sense).
    ///
    /// Note :The implementation making InstrumentComponentCache inherits from 
    /// Cache template class is faster than the one inheriting from 
    /// CalculationCache, or than having it as a member variable.
    template < class Arguments,
               class TComponent	= GenericInstrumentComponent<Arguments>,
			   template <typename> 
			   class TSmartPtr	= std::tr1::shared_ptr 
			 >
    class InstrumentComponentCache: public Cache< Arguments, 
                                                  TSmartPtr<TComponent>,
                                                  typename Arguments::Compare >
    {
    public:
        explicit InstrumentComponentCache(const bool cacheComponentsCalculations = false) :
            Cache(cacheComponentsCalculations ?
                std::tr1::bind( &CachedInstrumentComponent<Arguments, TComponent>::create, std::placeholders::_1) :
                std::tr1::bind( &TComponent::create, std::placeholders::_1) )
        {
        }
  
    };  //  InstrumentComponentCache

}   //  FlexYCF

#endif //__LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOMPONENTCACHE_H_INCLUDED