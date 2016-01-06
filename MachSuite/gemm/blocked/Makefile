bbgemm: bbgemm.c bbgemm.h ../../common/harness.c
	$(CC) $(CFLAGS) -o bbgemm $(SRCS) $(LFLAGS)

hls: bbgemm.c bbgemm.h
	vivado_hls hls.tcl

run: bbgemm input.data check.data
	./bbgemm input.data check.data

clean:
	rm -f bbgemm

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = bbgemm.c ../../common/harness.c
ACCEL_NAME = gemm-blocked
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=bbgemm
endif
include ../../common/Makefile.tracer
