ALADDIN v1.3.1 Public Release
============================================
[![build status](https://travis-ci.org/ysshao/ALADDIN.svg?branch=master)](https://travis-ci.org/ysshao/ALADDIN)

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
1. Boost Graph Library 1.55.0

The Boost Graph Library is a header-only library and does not need to be
built most of the time. But we do need a function that requires building
`libboost_graph` and `libboost_regex`. To build the two libraries:

* Download the Boost library from here:

`wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz`

* Unzip the tarball and set your `$BOOST_ROOT`:
```
tar -xzvf boost_1_55_0.tar.gz
cd boost_1_55_0
export BOOST_ROOT=/your/path/to/boost_1_55_0/
```
* Run:

`./bootstrap.sh`

* Install:

```
mkdir build
./b2 --build-dir=./build --with-graph --with-regex
```

* It will compile these two libraries to `$BOOST_ROOT/stage/lib`

2. GCC 4.7+ or more recent is *required*. However, we strongly suggest 4.9+, as we use many C++11 features.

3. LLVM 3.4.0 and Clang 3.4.0 64-bit. Notice that LLVM is not backward compatible. We recommend users to use the exact versions for LLVM and Clang.

4. zlib 1.2.8 or later.

5. LLVM IR Trace Profiler (LLVM-Tracer)
LLVM-Tracer is an LLVM compiler pass that instruments code in LLVM
machine-independent IR. It prints out a dynamic trace of your program, which can
then be taken as an input for Aladdin.

You can find  LLVM-Tracer here:

[https://github.com/ysshao/LLVM-Tracer.git]

To build LLVM-Tracer:

* Set `LLVM_HOME` to where you installed LLVM

```
export LLVM_HOME=/your/path/to/llvm
export PATH=$LLVM_HOME/bin/:$PATH
export LD_LIBRARY_PATH=$LLVM_HOME/lib/:$LD_LIBRARY_PATH
```

* Go to where you put LLVM-Tracer source code

```
cd /path/to/LLVM-Tracer
cd /path/to/LLVM-Tracer/full-trace
make
cd /path/to/LLVM-Tracer/profile-func
make
```

Build:
======
1. Set `$ALADDIN_HOME` to where put Aladdin source code.

`export ALADDIN_HOME=/your/path/to/aladdin`

2. Set `$BOOST_ROOT` to where put Boost source code and update `$LD_LIBRARY_PATH`

```
export BOOST_ROOT=/your/path/to/boost
export LD_LIBRARY_PATH=$BOOST_ROOT/stage/lib:$LD_LIBRARY_PATH
```

3. Build aladdin

```
cd $ALADDIN_HOME/common
make -j4
```

Run:
======
After you build Aladdin and LLVM-Tracer, you can use example SHOC programs in the SHOC
directory to test Aladdin.

This distribution of Aladdin models fixed-function
accelerators with scratchpad memory. Parameters like loop unrolling factors for
each loop and memory partition (or memory bandwith) for each array allocated in
your program. The functional units power models are based on OpenPDK 45nm and SRAM model from
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
interested in, set environment variable `WORKLOAD` to be the function names):

```
export WORKLOAD=triad
```
(if you have multiple functions you are interested in, separate with commas):
```
export WORKLOAD=md,md_kernel
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

It will generate a file called `dynamic_trace` under current directory.
We provide a python script to run the above steps automatically for SHOC.

```
cd $ALADDIN_HOME/SHOC/scripts/
python llvm_compile.py $ALADDIN_HOME/SHOC/triad triad
```

4. Config file

Aladdin takes user defined parameters to model corresponding accelerator
designs. We prepare an example of such config file at

```
cd $ALADDIN_HOME/SHOC/triad/example
cat config_example
```
```
partition,cyclic,a,8192,4,2  //cyclic partition array a, size 8192B, wordsize is 4B, with partition factor 2
partition,cyclic,b,8192,4,2  //cyclic partition array b, size 8192B, wordsize is 4B, with partition factor 2
partition,cyclic,c,8192,4,2  //cyclic partition array c, size 8192B, wordsize is 4B, with partition factor 2
unrolling,triad,triad,2        //unroll loop in triad, the loop label in triad.c, with unrolling factor 2
pipelining,1               //enable loop pipelining, applied to all loops
cycle_time,6               //clock period, currently we support 1, 2, 3, 4, 5, and 6ns.
```

The format of config file is:

```
partition,cyclic,array_name,array_size_in_bytes,wordsize,partition_factor
unrolling,function_name,loop_label,unrolling_factor
```

Two more configs:

```
partition,complete,array_name,array_size_in_bytes //convert the array into register
flatten,function_name,loop_increment_happend_at_line  //flatten the loop
```

Note that you need to explicitly config how to partition each array in your
source code. If you do not want to partition the array, declare it as
partition_factor 1 in your config file, like:

```
partition,cyclic,your-array,size-of-the-array,wordsize-of-each-element,1
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
1. This distribution of Aladdin models the datapath and local scratcpad memory
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


============================================
Sophia Shao,

shao@eecs.harvard.edu

Harvard University, 2014
