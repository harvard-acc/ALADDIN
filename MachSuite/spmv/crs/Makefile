crs: crs.c crs.h ../../common/harness.c
	$(CC) $(CFLAGS) -o crs $(SRCS) $(LFLAGS)

run: crs input.data check.data
	./crs input.data check.data

hls: crs.c crs.h
	vivado_hls hls.tcl

clean:
	rm -f crs

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = crs.c ../../common/harness.c
ACCEL_NAME = spmv-crs
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=spmv
endif
include ../../common/Makefile.tracer
