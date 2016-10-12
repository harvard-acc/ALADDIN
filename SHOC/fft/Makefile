SRCS=fft.c

ACCEL_NAME = fft
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=fft1D_512
endif
include ../common/Makefile.common
include ../common/Makefile.tracer
