SRCS=ss_sort.c

ACCEL_NAME = ss_sort
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=ss_sort
endif
include ../common/Makefile.common
include ../common/Makefile.tracer
