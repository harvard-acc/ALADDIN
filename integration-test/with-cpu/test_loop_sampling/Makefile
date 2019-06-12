ACCEL_NAME = test_loop_sampling
SRCS = test_loop_sampling.c $(ALADDIN_HOME)/gem5/sampling_interface.c
BMARK_SPECIFIC_CFLAGS=-DDMA_INTERFACE_V3
export WORKLOAD=test_loop_sampling

include ../../common/Makefile.tracer
include ../../common/Makefile.gem5
