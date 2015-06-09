stencil3d: stencil3d.c stencil3d.h ../../common/harness.c
	$(CC) $(CFLAGS) -o stencil3d $(SRCS) $(OTHER_SRCS)

hls: stencil3d.c stencil3d.h
	vivado_hls hls.tcl

run: stencil3d input.data check.data
	./stencil3d input.data check.data

clean:
	rm -f stencil3d

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = stencil3d.c ../../common/harness.c
ACCEL_NAME = stencil-stencil3d
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=stencil3d
endif
include ../../common/Makefile.tracer
