BENCHMARKS=\
	aes/aes \
	bfs/bulk \
	bfs/queue \
	fft/strided \
	fft/transpose \
	gemm/ncubed \
	gemm/blocked \
	kmp/kmp \
	md/knn \
	md/grid \
	nw/nw \
	sort/merge \
	sort/radix \
	spmv/crs \
	spmv/ellpack \
	stencil/stencil2d \
	stencil/stencil3d \
	viterbi/viterbi
#FIXME\
	backprop/backprop \

CFLAGS=-O3 -Wall -Wno-unused-label

.PHONY: build run all test clean trace-binary dma-trace-binary run-trace clean-trace

build:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b; done )

run:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b run; done )

generate:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b generate; done )

trace-binary:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b trace-binary; done )

dma-trace-binary:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b dma-trace-binary; done )

run-trace:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b run-trace; done )

gem5-cpu:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b gem5-cpu; done )

gem5-accel:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b gem5-accel; done )

### For regression tests
all: clean build generate run

test:
	$(MAKE) -C common/test
	$(MAKE) all CFLAGS="-O3 -Wall -Wno-unused-label -Werror"
	$(MAKE) all CFLAGS="-O3 -Wall -Wno-unused-label -Werror -std=c99"

clean:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b clean || exit ; done )

clean-trace:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b clean-trace || exit ; done )
