The class InstrumentComponent represents a unit of calculation that is
used by several intruments (e.g. the 1Y discount factor). It was introduced
to improve the efficiency of the computation-intensive part of calibration.
The idea is to avoid doing calculations that are the same but scattered accross
different instruments more than once. 
To achieve this goal, the GlobalComponentCache provides a global
access point to build or retrieve the concrete components (DiscountFactor, ForwardRate, etc...)
in the InstrumentComponent hierarchy. It encapsulates a factory and two levels of 
caching. 

1. Creational Caching
The first level of caching is entangled with the factory and ensures
that two equal components (in the sense that their construction arguments
are the same) are only built once. The second attempt to retrieve the same
component will return a shared pointer to the one that already exists.
In this way, unnecessary component building is achieved. This is particularly
useful for more complex components (ForwardRate, FloatingLeg), that are made of
several simpler and often equal components.

2. Calculation Caching
The second level of caching is related to calculation caching. It is optional 
and can be switched off on a component type basis (e.g. it is possible to enable 
caching for all component types except the FloatingLeg, or only cache the 
calculations of the DiscountFactor). This is done by setting the parameters of the
GlobalComponentCache to the wanted value. This behaviour is to allow fine-tune
control over the whole workflow of calculations and experimently test which
caching significantly improves performance.

Usage:

1. How to use the Cache?
        The following syntax should work:

            GlobalComponentCache gcc;
            gcc.get<ForwardRate>(ForwardRate::Arguments( ... ) );

        
2. How to create a new instrument component that can be cached ?
    a. Create a new concrete class, say MyComponentArgs that will 
        encapsulate the parameters for the new instrument component.
       This class should have an inner Compare functor so that this new
       instrument component arguments class objects are ordered. Any order
       will do, it is just for the cache to work).
       
    b. provide an implementation for GenericInstrumentComponent<MyComponentArgs>,
        probably in a MyComponent.cpp file. This will probably necessitate getters
        for MyComponentArgs
        
    c. add this new instrument component header to AllComponentsAndCaches.h

    d. add a shared_ptr to InstrumentComponentCache<NewInstrComponent> member variable in
        GlobalComponentCache. 
        Specialize the 'get' function with the new instrument component type.
        Add the code to clear this cache in the GlobalComponentCache::clear()
        Build this new component cache in the GlobalCache ctor, possibly taking a bool flag 
        specifty whether or not to use the calculation cache.
        
        
        
Instrument Components supported so far:
* DiscountFactor
* TenorDiscountFactor
* ForwardRate
* DiscountedForwardRate
* FloatingLeg and FloatingLegCashFlow