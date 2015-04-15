# This test runs the CortexSuite stitich benchmark through the gem5
# infrastructure.

.PHONY: test run

ifndef TARGET_DIR
export TARGET_DIR=`pwd`
endif
CORTEXSUITE_HOME = /group/vlsiarch/samxi/active_projects/cortexsuite
trace:
	$(MAKE) -C $(CORTEXSUITE_HOME)/vision/benchmarks/stitch/src/c autotrace
	cp $(CORTEXSUITE_HOME)/vision/benchmarks/stitch/src/c/dynamic_trace.gz .

run: test
	@echo off
	sh run.sh

test:
	$(MAKE) -C $(CORTEXSUITE_HOME)/vision/benchmarks/stitch/src/c gem5
