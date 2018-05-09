# GCC C++17 Parallel STL Offloading Execution Policy AKA 'par_offload'

This repository contains an experimental proof-of-concept of an
heterogeneous offload-capable Parallel STL. It introduces a new execution
policy which attempts to offload the execution to a device that supports HSA 1.0 Full Profile.
When the new 'par_offload' policy is used, the implementation tries to offload
the algorithm launch through the HSA runtime, and if it fails for
some reason, it gracefully falls back to host (CPU) execution.

*NOTE:* this project is in a preview stage and has received practically
no proper testing outside the included test suite. It is also not yet
optimized for any particular target.

## Usage

In your program add ```#include <experimental/par_offload>``` to your sources _after_ including
a PSTL implementation you want to fallback to.

Then build your program with the '-fpar_offload' switch:

```
g++ --std=c++17 -fpar_offload program.cc -o program
```

And run it normally:

```
./program
```

If you have a working HSA 1.0 Full Profile environment installed, it _might_
silently offload the algorithm's execution. To verify that offloading happens, you can set
the environment variable HSA_DEBUG to 1 to get verbose output.

## Offloaded Algorithms

Currently the following algorithms can be offloaded:

* copy_if
* max_element
* minmax_element
* reduce
* transform
* transform_reduce

## Things to Do

An incomplete list of improvements/fixes to do:

* Split the direct kernel invocation API to two functions; one that reflects
  the host functions name to a string (to get the placeholder function name),
  and one that takes an array of placeholder-address pairs.

## Credits

par_offload was developed by [Parmance](http://parmance.com) engineers
for [General Processor Technologies](http://www.generalprocessortech.com/)
who published the code to the open source community in May 2018.

It is heavily based on the excellent GCC HSA offloading work mostly done by
Martin Jambor and Martin Liška of [SUSE](http://suse.com).

