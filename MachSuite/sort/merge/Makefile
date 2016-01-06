merge: merge.c merge.h ../../common/harness.c
	$(CC) $(CFLAGS) -o merge $(SRCS) $(LFLAGS)

run: merge input.data check.data
	./merge input.data check.data

hls: merge.c merge.h
	vivado_hls hls.tcl

clean:
	rm -f merge

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = merge.c ../../common/harness.c
ACCEL_NAME = sort-merge
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=merge,mergesort
endif
include ../../common/Makefile.tracer
