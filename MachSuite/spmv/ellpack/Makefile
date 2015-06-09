ellpack: ellpack.c ellpack.h ../../common/harness.c
	$(CC) $(CFLAGS) -o ellpack $(SRCS) $(OTHER_SRCS)

run: ellpack input.data check.data
	./ellpack input.data check.data

hls: ellpack.c ellpack.h
	vivado_hls hls.tcl

clean:
	rm -f ellpack

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = ellpack.c ../../common/harness.c
ACCEL_NAME = spmv-ellpack
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=ellpack
endif
include ../../common/Makefile.tracer
