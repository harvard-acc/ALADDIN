bulk: bulk.c bulk.h ../../common/harness.c
	$(CC) $(CFLAGS) -o bulk $(SRCS) $(OTHER_SRCS)

run: bulk input.data check.data
	./bulk input.data check.data

hls: bulk.c bulk.h
	vivado_hls hls.tcl

clean:
	rm -f bulk

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = bulk.c ../../common/harness.c
ACCEL_NAME = bfs-bulk
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=bfs
endif
include ../../common/Makefile.tracer
