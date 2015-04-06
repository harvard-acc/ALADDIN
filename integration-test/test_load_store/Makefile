ACCEL_NAME = test_load_store
SRCS = test_load_store.cpp
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
export WORKLOAD=store_kernel

include ../common/Makefile.tracer
include ../common/Makefile.gem5

all: autotrace test
