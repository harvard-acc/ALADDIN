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

.PHONY: build run generate all test clean

build:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b; done )

run:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b run; done )

generate:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b generate; done )


### For regression tests
all: clean build generate run

test:
	$(MAKE) -C common/test
	$(MAKE) all CFLAGS="-O3 -Wall -Wno-unused-label -Werror"
	$(MAKE) all CFLAGS="-O3 -Wall -Wno-unused-label -Werror -std=c99"

clean:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b clean || exit ; done )
