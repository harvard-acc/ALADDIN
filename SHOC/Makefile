# This Makefile was adapted from the MachSuite benchmark suite.

BENCHMARKS=\
	bb_gemm \
	fft \
	md \
	pp_scan \
	reduction \
	ss_sort \
	stencil \
	triad

CFLAGS=-O3 -Wall -Wno-unused-label

.PHONY: build run all test clean trace-binary dma-trace-binary run-trace clean-trace

build:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b; done )

run:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b run; done )

trace-binary:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b trace-binary; done )

dma-trace-binary:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b dma-trace-binary; done )

run-trace:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b run-trace; done )

clean:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b clean || exit ; done )

clean-trace:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b clean-trace || exit ; done )

all: clean build run
