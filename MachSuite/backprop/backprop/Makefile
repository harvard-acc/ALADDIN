net: net.c net.h ../../common/harness.c
	$(CC) $(CFLAGS) -o net $(SRCS) $(OTHER_SRCS) -lm

run: net input.data check.data
	./net input.data check.data

hls: net.c net.h
	vivado_hls hls.tcl

clean:
	rm -f net

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = net.c ../../common/harness.c
ACCEL_NAME = backprop-backprop
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=sigmoid,update_layer,update,propagate_error_out,propagate_error_layer,update_weights,propagate_errors,comp_error,backprop
endif
include ../../common/Makefile.tracer
