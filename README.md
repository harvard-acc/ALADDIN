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

============================================
Requirements:
-------------------
1. Boost Graph Library 1.55.0

   The Boost Graph Library is a header-only library and does not need to be
   built most of the time. But we do need a function that requires building
   `libboost_graph` and `libboost_regex`. To build the two libraries:

   1. Download the Boost library from here:

      `wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz`

   2. Unzip the tarball and set your `$BOOST_ROOT`:
      ```
      tar -xzvf boost_1_55_0.tar.gz
      cd boost_1_55_0
      export BOOST_ROOT=/your/path/to/boost_1_55_0/
      ```
   3. Run:

      `./bootstrap.sh`

   4. Install:

      ```
      mkdir build
      ./b2 --build-dir=./build --with-graph --with-regex
      ```

   5. It will compile these two libraries to `$BOOST_ROOT/stage/lib`

2. Recent version of GCC including some C++11 features (GCC 4.5+ should be ok).
   We use GCC 4.8.1.

3. LLVM 3.4.0 and Clang 3.4.0 64-bit

4. LLVM IR Trace Profiler (LLVM-Tracer)
   LLVM-Tracer is an LLVM compiler pass that instruments code in LLVM
   machine-independent IR. It prints out a dynamic trace of your program, which can
   then be taken as an input for Aladdin.

   You can find  LLVM-Tracer here:

   [https://github.com/ysshao/LLVM-Tracer.git]

   To build LLVM-Tracer:

   1. Set `LLVM_HOME` to where you installed LLVM

       ```
       export LLVM_HOME=/your/path/to/llvm
       export PATH=$LLVM_HOME/bin/:$PATH
       export LD_LIBRARY_PATH=$LLVM_HOME/lib/:$LD_LIBRARY_PATH
      ```

   2. Go to where you put LLVM-Tracer source code

       **Skip this step if you want to use CMake to build.**

       If you plan to use the CMake to build Aladdin, we provide
       an automatic way of installing LLVM and LLVM-Tracer through
       CMake scripts. In this case, you don't need to install/build
       LLVM-Tracer separately

       ```
       cd /path/to/LLVM-Tracer
       cd /path/to/LLVM-Tracer/full-trace
       make
       cd /path/to/LLVM-Tracer/profile-func
       make
       ```

Build:
-----------------
  Aladdin currently supports two ways to build.

  1. Configure with CMake, then build with Make :

       If you are familiar with CMake, or you are interested in a script
       which can build out-of-source and offer automatical LLVM/Clang
       installation, choose [CMake](#build-with-cmake)

  2. Directly build with Makefile :

       If you want to use a simple and clean Makefile interface, you can
       build ALaddin directly with [Makefile](#build-with-makefile)


Build with CMake:
-----------------
CMake is a configure tool which allows you to out-of-source build.
Aladdin Requires CMake newer than 2.8.12. By default, CMake
searches for LLVM 3.4.


1. Set environment variables `BOOST_ROOT`, `TRACER_HOME`, `LLVM_HOME` to
   let Aladdin know where to search tools and libraries. Aladdin will
   not use environment variables to install LLVM.

   ```
   export BOOST_ROOT=/your/path/to/boost
   export TRACER_HOME=/your/path/to/LLVM-Tracer
   export LLVM_HOME=/your/path/to/llvm
   ```

2. Use CMake to configure Aladdin and build libraries and executables

   If you want to put the final executables and libraries on source
   directory, pass `-DBUILD_ON_SOURCE=TRUE` arguemnt to `cmake`.
   Otherwise, `-DBUILD_ON_SOURCE=FALSE`. The default is FALSE.

   ```
   mkdir /where/you/want/to/bulid/your/binary
   cd /where/you/want/to/bulid/your/binary
   cmake /your/path/to/Aladdin/source
   make -j4
   ```

3. (Optional) Run tests with `ctest` command

   **CAUTION : Running the full test will take a long time.**

   There are 1397 tests. On Intel Xeon E5-2650 2.60GHz,
   execution in 32 simultaneous processes takes 39 minutes,
   and execution in sequential takes 6.5 hours.


   **Suggest running part of them, like `"unit_test"` or `"triad"`.**


   Runs all the tests in sequential.
   ```
   ctest
   ```

   Runs all the tests in parallel. You can mix this option with test
   filtering option to speed up tests. The JOB_NUM here means the max
   job number to run simultaneously. Just like how you use `make -j NUM`
   ```
   ctest -j JOB_NUM
   ```

   Each test has name and the shell command. the name of test can be used
   to filter out the unwanted tests.


   Runs `unit_test` related tests. Use `-R` to pass the regular expression to
   match for.
   ```
   ctest -R unit_test
   ```

   Runs triad related tests.
   ```
   ctest -R triad
   ```

   If just want to know what -R `triad` matches. There is no actual execution of tests
   ```
   ctest -R triad --show-only
   ```

4. Available test names of CTest

   CMake collects available test cases and generate a list of configs,
   then generate bash shell scripts for each tests.
   `ctest` command for Aladdin executable usually runs these bash scripts.

   Besides tests using Aladdin, CMake also build tests which generates
   `dynamic_trace.gz`. It is named after just the case name without the
   config settings like `triad` or `fft`. CMake also applies the python
   script `plot_space.py` to plot design space. They are named in
   `X_data_plot` scheme.

   Of course, if multiple of tests are active at the same
   time, CTest will schedule all Aladdin tests according to predefined
   dependencies. Plot tests depend on Aladdin tests and Aladdin tests
   depend on LLVM-Tracer tests.

   ------------------------------------------------

   The available testcases:
      * bb_gemm, fft, md, pp_scan, reduction, stencil, triad, ss_sort, unit_test

   The generated design space goes through:

     Config               |   range
     ---------------------|---------------------------
     PARTITION_RANGE      |   [1 2 4 8 16 32 64]
     UNROLL_RANGE         |   [1 2 4 8 16 32 64]
     PIPELINE_RANGE       |   [1 2]
     CLOCK_RANGE          |   [1 6]


   This is a partial CTestName-ShellCommand table.
   The left side is the test names valid in CTest. While the right
   side is the corresponding shell commands. Just a subset of tests.
   Giving hints for how test names are comprised in Aladdin.


     ctest name                    |    shell command
     ------------------------------|---------------------------
     triad                         |    triad-instrumented
     triad_p2_u16_P2_1ns           |    triad.sh 2 16 2 1
     triad_p2_u2_P1_6ns            |    triad.sh 2 2 1 6
     triad_p4_u8_P2_1ns            |    triad.sh 4 8 2 1
     triad_data_plot               |    triad_plot.sh
     fft_p4_u8_P2_1ns              |    fft.sh 4 8 2 1
     bb_gemm_p4_u8_P2_1ns          |    bb_gemm.sh 4 8 2 1
     unit_test_init_base_address   |    unit_test_init_base_address
     unit_test_store_buffer        |    unit_test_store_buffer


5. Available CMake Settings

   ```
   -DTRACER_DIR=/where/your/llvm/install  (default : $TRACER_HOME)
     You may denote the path of LLVM-Tracer source code by this option.
     After the source code of LLVM-Tracer is found by CMake script of
     Aladdin, CMake script of Aladdin will reuse CMake scripts of
     LLVM-Tracer and build LLVM-Tracer on Aladdin's binary directory.
     If this option is not defined, environment variable TRACER_HOME
     will be used.

   -DBOOST_ROOT=/where/your/boost/install  (default : $BOOST_ROOT)
     You can manually specify the installation path of boost library
     by this CMake options. If this option is not defined, environment
     variable BOOST_ROOT will be used.

   ```

6. Available CMake Settings from LLVM-Tracer

   ```
   -DLLVM_ROOT=/where/your/llvm/install   (default : $LLVM_HOME)
     You may denote the path of LLVM to find/install by this option. If
     this option is not defined, environment variable LLVM_HOME will be
     used.

   -DLLVMC_FLAGS="additional cflags"        (default : empty)
     This option offers you the power to pass additional cflags
     to Clang while building LLVM IR.

   -DTRACER_LD_FLAGS="additional ldflags"   (default : empty)
     This option offers you the power to pass additional ldflags
     to g++ while linking instrumented LLVM IR to executable.

   -DLLVM_RECOMMEND_VERSION="3.4", "3.5"    (default : 3.4)
     LLVM-Tracer supports both LLVM 3.4 and 3.5. It uses LLVM 3.4 by
     default, but you can manually specify the LLVM version to use.

   -DGCC_INSTALL_PREFIX="/your/gcc/location"       (default : empty)
     Clang++ requires stdc++ of GCC. This option denotes the
     path to search libstdc++ and it's headers. If this option
     is undefined, Clang++ will apply it's default settings.
     In LLVM auto-installer, this option will be used as the
     default search path by the installed Clang.

   -DCMAKE_PREFIX_PATH="/your/toolchain/location"  (default : empty)
     This CMake option denotes the additional path to search
     dependent tools. In a sitution where you want to use LLVM
     auto-installer and have pyhton 2.7 installed outside from
     system directories, you should manually specify location
     of python via this option.

   -DAUTOINSTALL=TRUE,FALSE    (default : FALSE)
     By this option, CMake scripts will automatically download, build and
     install LLVM for you if finds no LLVM installation. Using this
     function requires tar-1.22 or newer to extract xz format.

     The default installation path is under /your/build_dir/lib/llvm-3.x.
     You can manually define the installation path by
     -DLLVM_ROOT=/where/to/install.

     The default installation version will be 3.4. You can define
     the installation version by -DLLVM_RECOMMEND_VERSION="3.5" or "3.4".
     This auto install script will try to use the latest patch version
     according to cmake-scripts/AutoInstall/LLVMPatchVersion.cmake

   -DCMAKE_BUILD_TYPE=None,Debug,Release    (default : None)
     You can choose one from three of bulid types.
     The build type here is used to hint CMake what the optimization
     level is and what the debug level is. Debug type will invoke -g3
     to gcc. Release type will invoke -O2 to gcc.

   -DBUILD_ON_SOURCE=TRUE,FALSE    (default : FALSE)
     By assign this option, CMake will build fulltrace.so &
     trace_logger.llvm under the source directory.
     Other llvm bitcode and object files still remain in the build directory.

   -DDYN_LINK_TRACE_CODE=TRUE,FALSE    (default : FALSE)
     By turn on this option, all the LLVM-Tracer instrumented code
     are linked with dynamic libraries.
   ```

Build with Makefile:
--------------------
1. Set `$ALDDIN_HOME` to where put Aladdin source code.

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
----
After you build Aladdin and LLVM-Tracer, you can use example SHOC programs in the SHOC
directory to test Aladdin.

This distribution of Aladdin models fixed-function
accelerators with scratchpad memory. Parameters like loop unrolling factors for
each loop and memory partition (or memory bandwith) for each array allocated in
your program. The functional units power models are based on OpenPDK 45nm and SRAM model from
CACTI 5.3.

Example program: triad
----------------------
  We prepare a script to automatically run all the steps above: generating
  trace, config file and run Aladdin here:

  `$ALADDIN_HOME/SHOC/scripts/run_aladdin.py`

  You can run:

  ```
  cd $ALADDIN_HOME/SHOC/scripts
  python run_aladdin.py triad 2 2 1 6
  ```

  It will start Aladdin run at `$ALADDIN_HOME/SHOC/triad/sim/p2_u2_P1_6ns`

  The last three parameters are unrolling and partition factors and enabling
  loop pipelining in the config
  files. You can sweep these parameters easily with Aladdin.

Step-by-step:
----------------------
1. Go to `$ALADDIN_HOME/SHOC/triad`
2. Run LLVM-Tracer to generate a dynamic LLVM IR trace
   1. Both Aladdin and LLVM-Tracer track regions of interest inside a program. In the
      triad example, we want to analyze the triad kernel instead of the setup
      and initialization work done in main. To tell LLVM-Tracer the functions we are
      interested in, set enviroment variable `WORKLOAD` to be the function names):

    ```
    export WORKLOAD=triad
    ```
    (if you have multiple functions you are interested in, separate with commas):
    ```
    export WORKLOAD=md,md_kernel
    ```
   2. Generate LLVM IR:

    `clang -g -O1 -S -fno-slp-vectorize -fno-vectorize -fno-unroll-loops -fno-inline -emit-llvm -o triad.llvm triad.c`

   3. Run LLVM-Tracer pass:
      Before you run, make sure you already built LLVM-Tracer.
      Set `$TRACER_HOME` to where you put LLVM-Tracer code.

     ```
     export TRACER_HOME=/your/path/to/LLVM-Tracer
     opt -S -load=$TRACER_HOME/full-trace/full_trace.so -fulltrace triad.llvm -o triad-opt.llvm
     llvm-link -o full.llvm triad-opt.llvm $TRACER_HOME/profile-func/trace_logger.llvm
     ```

   4. Generate machine code:

     ```
     llc -filetype=asm -o full.s full.llvm
     gcc -fno-inline -o triad-instrumented full.s -lm -lz
     ```

   5. Run binary:

     `./triad-instrumented`

     It will generate a file called `dynamic_trace` under current directory.
     We provide a python script to run the above steps automatically for SHOC.

     ```
     cd $ALADDIN_HOME/SHOC/scripts/
     python llvm_compile.py $ALADDIN_HOME/SHOC/triad triad
     ```

3.config file

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
  unrolling,triad,5,2        //unroll loop in triad, define at line 5 in triad.c, with unrolling factor 2
  pipelining,1               //enable loop pipelining, applied to all loops
  cycle_time,6               //clock period, currently we support 1, 2, 3, 4, 5, and 6ns.
  ```

  The format of config file is:

  ```
  partition,cyclic,array_name,array_size_in_bytes,wordsize,partition_factor
  unrolling,function_name,loop_increment_happened_at_line,unrolling_factor
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
   $ALADDIN_HOME/common/aladdin triad $ALADDIN_HOME/SHOC/triad/dynamic_trace config_example
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

5. Design Space
   We provide a simple script to show how to run Aladdin to generate the design
   space of a workload. The script is

  `$ALADDIN_HOME/SHOC/scripts/gen_space.py`

   You can run:

   ```
   cd $ALADDIN_HOME/SHOC/scripts
   python gen_space.py triad
   ```

   You can replace triad with any SHOC benchmark that we provide. It will
   automatically generate the config file and trigger Aladdin.

   It sweeps unrolling and partition factors, as well as turn on and off loop
   pipelining, to generate a power-performance-area design space. At the end of
   the script, if you have matplotlib installed, it will generate 6 plots of
   design spaces in terms of total-power vs cycles, fu-power vs cycles,
   mem-power vs cycles, total-area vs cycles, fu-area vs cycles, and mem-area vs
   cycles.

Caveats
-------
1. This distribution of Aladdin models the datapath and local scratcpad memory
of accelerators but not includes the rest of the memory hierarchy.
We are working on providing a release of Aladdin with a memory hierarchy. At the
same time, if you are interested in doing your customized integration, we will
provide a user`s guide on how to do the integration.

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
