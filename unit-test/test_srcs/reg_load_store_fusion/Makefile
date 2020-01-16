SRCS=reg_load_store_fusion.c

ACCEL_NAME = reg_load_store_fusion
TEST_BIN = $(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=rmw_fusion
endif
include ../Makefile.tracer
