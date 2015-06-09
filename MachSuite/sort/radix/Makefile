radix: radix.c radix.h ../../common/harness.c
	$(CC) $(CFLAGS) -o radix $(SRCS) $(OTHER_SRCS)

run: radix input.data check.data
	./radix input.data check.data

hls: radix.c radix.h
	vivado_hls hls.tcl

clean:
	rm -f radix

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = radix.c ../../common/harness.c
ACCEL_NAME = sort-radix
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifdef
  export WORKLOAD=local_scan,sum_scan,last_step_scan,init,hist,update,ss_sort
endif
include ../../common/Makefile.tracer
