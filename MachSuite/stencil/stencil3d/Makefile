KERN=stencil
ALG=stencil3d

CFLAGS?=-O3 -Wall -Wno-unused-label

SRCS=$(KERN).c local_support.c ../../common/support.c
FILES=$(SRCS) $(KERN).h ../../common/support.h

$(KERN): $(FILES) ../../common/harness.c
	$(CC) $(CFLAGS) -I../../common -o $(KERN) $(SRCS) ../../common/harness.c

run: $(KERN) input.data check.data
	./$(KERN) input.data check.data

generate: $(FILES) generate.c
	$(CC) $(CFLAGS) -I../../common -o generate $(SRCS) generate.c
	./generate

hls: $(KERN).c $(KERN).h
	vivado_hls hls.tcl

clean:
	rm -f $(KERN) generate output.data

ACCEL_NAME = stencil_stencil3d
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=stencil3d
endif
include ../../common/Makefile.tracer
include ../../common/Makefile.gem5
