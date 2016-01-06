gemm: gemm.c gemm.h ../../common/harness.c
	$(CC) $(CFLAGS) -o gemm $(SRCS) $(LFLAGS)

hls: gemm.c gemm.h
	vivado_hls hls.tcl

run: gemm input.data check.data
	./gemm input.data check.data

clean:
	rm -f gemm

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = gemm.c ../../common/harness.c
ACCEL_NAME = gemm-ncubed
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=gemm
endif
include ../../common/Makefile.tracer
