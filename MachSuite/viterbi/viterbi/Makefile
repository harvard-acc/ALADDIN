viterbi: viterbi.c viterbi.h ../../common/harness.c
	$(CC) $(CFLAGS) -o viterbi $(SRCS) $(LFLAGS)

run: viterbi input.data check.data
	./viterbi input.data check.data

hls: viterbi.c viterbi.h
	vivado_hls hls.tcl

clean:
	rm -f viterbi

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = viterbi.c ../../common/harness.c
ACCEL_NAME = viterbi-viterbi
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=viterbi
endif
include ../../common/Makefile.tracer
