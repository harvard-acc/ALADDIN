needwun: needwun.c needwun.h ../../common/harness.c
	$(CC) $(CFLAGS) -o needwun $(SRCS) $(OTHER_SRCS)

run: needwun input.data check.data
	./needwun input.data check.data

hls: needwun.c needwun.h
	vivado_hls hls.tcl

clean:
	rm -f needwun

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = needwun.c ../../common/harness.c
ACCEL_NAME = nw-nw
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=needwun
endif
include ../../common/Makefile.tracer
