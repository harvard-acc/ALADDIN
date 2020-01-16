SRCS=memory_ambiguation.c

ACCEL_NAME = memory_ambiguation
TEST_BIN = $(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=test_independent,test_indirect
endif
include ../Makefile.tracer
