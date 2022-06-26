/*****************************************************************************

    Todo: - Add header file description
    
    @Originator		Nicolas Maury
    
    Copyright (C) Lloyds TSB Group plc 2007-08 All Rights Reserved

*****************************************************************************/

#ifndef __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOMPONENT_H_INCLUDED
#define __LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOMPONENT_H_INCLUDED

#include "Gradient.h"


namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( BaseModel )
	FWD_DECLARE_SMART_PTRS( CurveType )

    class GlobalComponentCache;

    ///  A common interface for all concrete instrument components
    class InstrumentComponent
    {
    public:
        virtual ~InstrumentComponent(){ }

        /// Returns the value of the instrument component
        /// in the specified model
        virtual double getValue(BaseModel const& baseModel) = 0;

        /// Computes the gradient of this instrument component.
        virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd) = 0;

		/// Computes the gradient of this instrument component, relative
		/// to the variables of the specified curve type
        virtual void accumulateGradient(BaseModel const& baseModel, 
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType) = 0;
		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread) = 0;
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread) = 0;
        /// "Updates" the instrument component. 
        ///  Note: allows a CachedInstrumentComponent to "reset" its cached values.
        ///  Also, for instrument components made of other instrument components,
        ///  this function provides a mean to update itself and forward the 
        ///  update to its constituents.
        virtual void update() = 0;

        /// Sets the global component cache
        static void setGlobalComponentCache(GlobalComponentCache * globalComponentCache)
        {
            s_globalComponentCache = globalComponentCache;
			if( s_globalComponentCache == NULL )
			{
				s_useCache = false;
			}
			else
			{
				s_useCache = true;
			}
        }

        /// Gets the global component cache
        static GlobalComponentCache * getGlobalComponentCache() 
        {
            return s_globalComponentCache;
        }

		///	Indicates whether to use the cache or not
		static bool getUseCacheFlag() 
		{
			return s_useCache;
		}

        virtual std::ostream& print(std::ostream& out) const 
        { 
            out << "Instrument Component" << std::endl;
            return out; 
        }

		//	Nested class to temporarily switch the static "use cache" flag
		//	at a given scope level ensuring it is automatically set	back 
		//	to its previous value when the instance of this class goes out
		//	of scope.
		//	Note: this is robust in presence of exceptions.
		struct CacheScopeSwitcher: private DevCore::NonCopyable
		{
		public:
			explicit CacheScopeSwitcher(const bool flag = false):
				m_previousFlag(InstrumentComponent::getUseCacheFlag())
			{
				FlexYCF::InstrumentComponent::setUseCacheFlag(flag);
			}

			~CacheScopeSwitcher()
			{
				FlexYCF::InstrumentComponent::setUseCacheFlag(m_previousFlag);
			}

		private:
			const bool m_previousFlag;
		};

    private:
		/// Sets the cache flag - now private to oblige clients to use the CacheScopeSwitcher
		static void setUseCacheFlag(const bool useCache)
		{
			s_useCache = useCache;
		}

        // GlobalComponentCache should be model-specific
        static GlobalComponentCache*	s_globalComponentCache;
		static bool						s_useCache;
        
    };  //  InstrumentComponent

    DECLARE_SMART_PTRS( InstrumentComponent )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const InstrumentComponentPtr instrumentComponent)
		{
			return instrumentComponent->print(out);
		}
	}

}   //  FlexYCF



namespace FlexYCF
{

    FWD_DECLARE_SMART_PTRS( BaseModel )
	FWD_DECLARE_SMART_PTRS( CurveType )

    class GlobalComponentCache;

    ///  A common interface for all concrete instrument components
    class MultiCcyInstrumentComponent
    {
    public:
        virtual ~MultiCcyInstrumentComponent(){ }

        /// Returns the value of the instrument component
        /// in the specified model
        virtual double getValue(BaseModel const& domModel, BaseModel const& forModel, double fx) = 0;

        /// Computes the gradient of this instrument component.
        virtual void accumulateGradient(BaseModel const& domModel, BaseModel const& forModel, double fx,
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd) = 0;

		/// Computes the gradient of this instrument component, relative
		/// to the variables of the specified curve type
        virtual void accumulateGradient(BaseModel const& domModel, BaseModel const& forModel, double fx,
                                        double multiplier, 
                                        GradientIterator gradientBegin, 
                                        GradientIterator gradientEnd,
										const CurveTypeConstPtr& curveType) = 0;
				
		virtual void accumulateGradientConstantDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier,
										double fx,
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread) 
		{
			USE(baseModel)
			USE(dfModel)
			USE(multiplier)
			USE(fx)
			USE(gradientBegin)
			USE(gradientEnd)
			USE(spread)
		}
		
		virtual void accumulateGradientConstantTenorDiscountFactor(BaseModel const& baseModel, BaseModel const& dfModel,
                                        double multiplier, 
										double fx,
                                        GradientIterator gradientBegin, 
										GradientIterator gradientEnd,
										bool spread) 
		{
			USE(baseModel)
			USE(dfModel)
			USE(multiplier)
			USE(fx)
			USE(gradientBegin)
			USE(gradientEnd)
			USE(spread)
		}
		

        /// "Updates" the instrument component. 
        ///  Note: allows a CachedInstrumentComponent to "reset" its cached values.
        ///  Also, for instrument components made of other instrument components,
        ///  this function provides a mean to update itself and forward the 
        ///  update to its constituents.
        virtual void update() = 0;

        /// Sets the global component cache
        static void setGlobalComponentCache(GlobalComponentCache * globalComponentCache)
        {
            s_globalComponentCache = globalComponentCache;
        }

        /// Gets the global component cache
        static GlobalComponentCache * getGlobalComponentCache() 
        {
            return s_globalComponentCache;
        }

		///	Indicates whether to use the cache or not
		static bool getUseCacheFlag() 
		{
			return s_useCache;
		}

        virtual std::ostream& print(std::ostream& out) const 
        { 
            out << "Multi Ccy Instrument Component" << std::endl;
            return out; 
        }

		//	Nested class to temporarily switch the static "use cache" flag
		//	at a given scope level ensuring it is automatically set	back 
		//	to its previous value when the instance of this class goes out
		//	of scope.
		//	Note: this is robust in presence of exceptions.
		struct CacheScopeSwitcher: private DevCore::NonCopyable
		{
		public:
			explicit CacheScopeSwitcher(const bool flag = false):
				m_previousFlag(MultiCcyInstrumentComponent::getUseCacheFlag())
			{
				FlexYCF::MultiCcyInstrumentComponent::setUseCacheFlag(flag);
			}

			~CacheScopeSwitcher()
			{
				FlexYCF::MultiCcyInstrumentComponent::setUseCacheFlag(m_previousFlag);
			}

		private:
			const bool m_previousFlag;
		};

    private:
		/// Sets the cache flag - now private to oblige clients to use the CacheScopeSwitcher
		static void setUseCacheFlag(const bool useCache)
		{
			s_useCache = useCache;
		}

        // GlobalComponentCache should be model-specific
        static GlobalComponentCache*	s_globalComponentCache;
		static bool						s_useCache;
        
    };  //  InstrumentComponent

    DECLARE_SMART_PTRS( MultiCcyInstrumentComponent )

	namespace
	{
		std::ostream& operator<<(std::ostream& out, const MultiCcyInstrumentComponentPtr instrumentComponent)
		{
			return instrumentComponent->print(out);
		}
	}

}   //  FlexYCF
#endif //__LIBRARY_PRICERS_FLEXYCF_INSTRUMENTCOMPONENT_H_INCLUDED