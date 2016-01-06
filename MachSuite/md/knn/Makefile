md: md.c md.h ../../common/harness.c
	$(CC) $(CFLAGS) -o md $(SRCS) $(LFLAGS)

run: md input.data check.data
	./md input.data check.data

hls: md.c md.h
	vivado_hls hls.tcl

clean:
	rm -f md

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = md.c ../../common/harness.c
ACCEL_NAME = md-knn
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=md_kernel
endif
include ../../common/Makefile.tracer
