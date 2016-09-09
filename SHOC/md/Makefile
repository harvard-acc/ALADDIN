SRCS=md.c

ACCEL_NAME = md
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=md
endif
include ../common/Makefile.common
include ../common/Makefile.tracer
