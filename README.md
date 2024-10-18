# EmCore
C++ core library for embedded programming.

The aim of this library is to provide a set of tiny RAM and Flash footprint classes.
Templates are used to avoid usage of dynamic heap memory. 

EmList class make use of heap memory allocation. You typically declare list object as globals during setup. To avoid heap fragmentation you might avoid using EList objects within loops. 

