SRCS=triad.c

ACCEL_NAME = triad
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=triad
endif
include ../common/Makefile.common
include ../common/Makefile.tracer
