SRCS=stencil.c

ACCEL_NAME = stencil
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=stencil
endif
include ../common/Makefile.common
include ../common/Makefile.tracer
