fft: fft.c fft.h ../../common/harness.c
	$(CC) $(CFLAGS) -o fft $(SRCS) $(LFLAGS) -lm

run: fft input.data check.data
	./fft input.data check.data

hls: fft.c fft.h
	vivado_hls hls.tcl

clean:
	rm -f fft

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = fft.c ../../common/harness.c
ACCEL_NAME = fft-transpose
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=fft1D_512
endif
include ../../common/Makefile.tracer
