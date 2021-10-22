ALADDIN v2.0 Public Release
============================================
[![harvard-acc](https://circleci.com/gh/harvard-acc/ALADDIN.svg?style=shield)](https://circleci.com/gh/harvard-acc/ALADDIN)

Aladdin is a pre-RTL, power-performance simulator for fixed-function
accelerators.

Please read the licence distributed with this release in the same directory as
this file.

If you use Aladdin in your research, please cite:

Aladdin: A Pre-RTL, Power-Performance Accelerator Simulator Enabling Large
Design Space Exploration of Customized Architectures,
Yakun Sophia Shao, Brandon Reagen, Gu-Yeon Wei and David Brooks,
International Symposium on Computer Architecture, June, 2014

Requirements:
=============
We *highly* recommend that users use the
[Docker image](https://hub.docker.com/repository/docker/xyzsam/gem5-aladdin),
as it has all the required dependencies installed and the environment prepared.
If you do not or cannot use Docker, then read on.

### Boost Graph Library 1.55.0 ###

The Boost Graph Library is a header-only library and does not need to be
built most of the time. But we use the Boost graph library, and this does
require building the dynamic library `libboost_graph`. The Boost library
can be downloaded
[here](https://www.boost.org/users/history/version_1_55_0.html).
Build and installation directions are
[here](https://www.boost.org/doc/libs/1_72_0/more/getting_started/unix-variants.html).

You can pick any directory to install the libraries. Then, set the `BOOST_ROOT`
environment variable to that location, such that `$BOOST_ROOT/include` contains
the headers and `$BOOST_ROOT/lib` contains the libraries.

### GCC ###
GCC 4.7 or more recent is *required*. However, we strongly suggest 4.9+, as
we use many C++11 features.

### LLVM and Clang dev headers and libraries ###

* Aladdin v2.0+: LLVM and Clang 6.0.0.
* Aladdin pre v2.0: LLVM and Clang 3.4.0.

In both cases, they should be built from source with the `Debug` build type.
Download [here](http://releases.llvm.org/download.html). More detailed
instructions for installation can be found in the LLVM-Tracer instructions.

### zlib ###
zlib 1.2.11 or later is required.

### LLVM Tracer ###

LLVM-Tracer is an LLVM compiler pass that instruments code in LLVM
machine-independent IR. It prints out a dynamic trace of your program, which is
then used as an input for Aladdin.

Download and install [here](https://github.com/ysshao/LLVM-Tracer.git).

### Environment variables ###

* `ALADDIN_HOME`: where the root of the Aladdin source tree is located.
* `LLVM_HOME`: where LLVM is installed.
* `TRACER_HOME`: where the LLVM-Tracer shared libraries are installed
* `BOOST_ROOT`: where Boost is installed
* `PATH`: prepend `LLVM_HOME/lib`
* `LD_LIBRARY_PATH`: prepend `$BOOST_ROOT/lib` and `$TRACER_HOME/lib`

Build:
======
After setting the environment variables:
```
cd $ALADDIN_HOME/common
make
```

Run:
======
After you build Aladdin and LLVM-Tracer, you can use the provided programs in
the SHOC and MachSuite directories to test Aladdin.

This distribution of Aladdin models fixed-function accelerators with scratchpad
memory. Parameters like loop unrolling factors for each loop and memory
partition (or memory bandwith) for each array allocated in your program. The
functional units power models are based on OpenPDK 45nm and SRAM model from
CACTI 5.3.

In the following sessions, we use the `triad` benchmark in `SHOC` benchmark
suite to describe how to use Aladdin to generate power, performance, and area
estimates for a particular accelerator design.

Step-by-step:
----------------------
1. Go to `$ALADDIN_HOME/SHOC/triad`
2. Do `make run-trace`, which will generate a dynamic LLVM IR trace using LLVM-Tracer.
3. Internally, the `make` script wraps up the following parts:

* Declare functions to be accelerated. To tell LLVM-Tracer the functions we are
interested in, set the environment variable `WORKLOAD` to be the function names):

```
export WORKLOAD=triad
```
This will cause LLVM-Tracer to trace the function `triad` and all functions
called from it. If you have more than one function in your program you want to
trace, just add it to WORKLOAD:
```
export WORKLOAD=triad,md
```
* Generate LLVM IR:

`clang -g -O1 -S -fno-slp-vectorize -fno-vectorize -fno-unroll-loops -fno-inline -emit-llvm -o triad.llvm triad.c`

* Run LLVM-Tracer pass:
Before you run, make sure you already built LLVM-Tracer.
Set `$TRACER_HOME` to where you put LLVM-Tracer code.

```
export TRACER_HOME=/your/path/to/LLVM-Tracer
opt -S -load=$TRACER_HOME/full-trace/full_trace.so -fulltrace triad.llvm -o triad-opt.llvm
llvm-link -o full.llvm triad-opt.llvm $TRACER_HOME/profile-func/trace_logger.llvm
```

* Generate machine code:

```
llc -filetype=asm -o full.s full.llvm
gcc -fno-inline -o triad-instrumented full.s -lm -lz
```

* Run binary:

`./triad-instrumented`

It will generate a file called `dynamic_trace` under the current directory.

4. Configuration

Aladdin takes a set of user defined parameters to model the corresponding
accelerator designs. For example, here is one possible config for `triad`:

```c
partition,cyclic,a,8192,4,2  // cyclic partition array a, size 8192B, wordsize is 4B, with partition factor 2
partition,cyclic,b,8192,4,2  // cyclic partition array b, size 8192B, wordsize is 4B, with partition factor 2
partition,cyclic,c,8192,4,2  // cyclic partition array c, size 8192B, wordsize is 4B, with partition factor 2
unrolling,triad,triad,2      // unroll loop in triad, the loop label in triad.c, with unrolling factor 2
pipeline,triad,1             // enable loop pipelining on the "triad" loop.
cycle_time,6                 // clock period. Currently we support 1, 2, 3, 4, 5, and 6ns.
```

The configuration file accepts several different optimization directives:
`partition`,`unrolling`,`flatten`,`pipeline`, and `cycle_time`. The format of
each is shown below:

**Array partitioning** can be `cyclic`, `block`, or `complete`:

```c
// Cyclic partitioning of the array.
partition,cyclic,array_name,array_size_in_bytes,wordsize,partition_factor
// Block partitioning of the array.
partition,block,array_name,array_size_in_bytes,wordsize,partition_factor
// Convert the array into registers.
partition,complete,array_name,array_size_in_bytes
```
Note that you need to explicitly config how to partition each array in your
source code. If you do not want to partition the array, declare it as
partition_factor 1 in your config file, like:

**Loop unrolling**: each loop must be labeled in the code in order for these
directives to work.

```c
// Unroll the specified loop by the given factor.
unrolling,function_name,loop_label,unrolling_factor
// Flatten the loop completely.
flatten,function_name,loop_label
```

**Loop pipelining**: Aladdin pre v2.0 supported a "global pipelining" directive
(`pipelining`) that would attempt to pipeline every single loop in the code.
This has been deprecated and now replaced with a per-loop pipelining directive.

```c
pipeline,loop_name
```

4. Run Aladdin

Aladdin takes three parameters:
a. benchmark name
b. path to the dynamic trace generated by LLVM-Tracer
c. config file
Now you are ready to run Aladdin by:

```
cd $ALADDIN_HOME/SHOC/triad/example
$ALADDIN_HOME/common/aladdin triad ../dynamic_trace.gz config_example
```

Aladdin will print out the different stages of optimizations and scheduling as
it runs. In the end, Aladdin prints out the performance, power and area
estimates for this design, which is also saved at <bench_name>_summary
(triad_summary) in this case.

Aladdin will generate some files during its execution. One file you might
be interested is
`<bench_name>_stats`
which profiles the dynamic activities as accelerator is running. Its format:

```
line1: cycles,<cycle count>,<# of nodes in the trace>
line2: <cycle count>,<function-name-mul>,<function-name-add>,<each-partitioned-array>,....
line3: <cycle 0>,<# of mul happend from functiona-name at cycle 0>,..
line4: ...
```

A corresponding dynamic power trace is
`<bench_name>_stats_power`

Caveats
-------
1. This distribution of Aladdin models the datapath and local scratchpad memory
of accelerators but does not include the rest of the memory hierarchy.  If you
are interested in an SoC simulator that includes support for caches as well as
CPUs (and more), please take a look at
[gem5-Aladdin](https://github.com/harvard-acc/gem5-aladdin).

2. No Function Pipelining:

This distribution of Aladdin does not model function pipelining. In this
case, if a program has multiple functions built into accelerators, This
distribution of Aladdion assumes only function executes at a time.

3. Power Model Library:

This distribution of Aladdin characterizes power using OpenPDK 45nm
technology. The characterized power for functional units are in
`utils/power_delay.h`. If you are interested in trying different technologies,
modify the constants there with your power delay characteristics and then
recompile Aladdin. We will be releasing the microbenchmark set that we used to
do power characterization soon.

-------

Sam Xi, Yuan Yao, and Sophia Shao.

If you have any questions, please send an email to gem5-Aladdin-users@googlegroups.com

Original release: Harvard University, 2014
