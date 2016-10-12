SRCS=pp_scan.c

ACCEL_NAME = pp_scan
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=pp_scan
endif
include ../common/Makefile.common
include ../common/Makefile.tracer
