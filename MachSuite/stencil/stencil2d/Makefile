stencil: stencil.c stencil.h ../../common/harness.c
	$(CC) $(CFLAGS) -o stencil $(SRCS) $(OTHER_SRCS)

hls: stencil.c stencil.h
	vivado_hls hls.tcl

run: stencil input.data check.data
	./stencil input.data check.data

clean:
	rm -f stencil

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = stencil.c ../../common/harness.c
ACCEL_NAME = stencil-stencil2d
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=stencil
endif
include ../../common/Makefile.tracer
