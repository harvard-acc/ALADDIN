kmp: kmp.c kmp.h ../../common/harness.c
	$(CC) $(CFLAGS) -o kmp $(SRCS) $(OTHER_SRCS)

run: kmp input.data check.data
	./kmp input.data check.data

hls: kmp.c kmp.h
	vivado_hls hls.tcl

clean:
	rm -f kmp

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = kmp.c ../../common/harness.c
ACCEL_NAME = kmp-kmp
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=kmp
endif
include ../../common/Makefile.tracer
