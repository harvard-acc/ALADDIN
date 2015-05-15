# This test runs the CortexSuite stitich benchmark through the gem5
# infrastructure.

.PHONY: test run

ifndef TARGET_DIR
export TARGET_DIR=`pwd`
endif
ifeq ($(CORTEXSUITE_HOME),)
  $(error CORTEXSUITE_HOME is not set!)
  endif
trace:
	$(MAKE) -C $(CORTEXSUITE_HOME)/vision/benchmarks/stitch/src/c autotrace
	cp $(CORTEXSUITE_HOME)/vision/benchmarks/stitch/src/c/dynamic_trace.gz .

run: test
	@echo off
	sh run.sh

test:
	$(MAKE) -C $(CORTEXSUITE_HOME)/vision/benchmarks/stitch/src/c gem5
