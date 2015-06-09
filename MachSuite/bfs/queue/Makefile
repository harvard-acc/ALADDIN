queue: queue.c queue.h ../../common/harness.c
	$(CC) $(CFLAGS) -o queue $(SRCS) $(OTHER_SRCS)

run: queue input.data check.data
	./queue input.data check.data

hls: queue.c queue.h
	vivado_hls hls.tcl

clean:
	rm -f queue

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = queue.c ../../common/harness.c
ACCEL_NAME = bfs-queue
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=bfs
endif
include ../../common/Makefile.tracer
