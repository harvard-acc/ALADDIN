SRCS=reduction.c

ACCEL_NAME = reduction
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=reduction
endif
include ../common/Makefile.common
include ../common/Makefile.tracer
