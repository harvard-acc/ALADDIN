# This test runs the aes benchmark through the gem5 infrastructure and performs
# the output validation to show that the infrastructure is modeling all loads
# and stores correctly.

ACCEL_NAME = test_double_buffering
SRCS = test_double_buffering.c
BMARK_SPECIFIC_CFLAGS=-DDMA_INTERFACE_V3
export WORKLOAD=kernel_loop

include ../../common/Makefile.tracer
include ../../common/Makefile.gem5
