BENCHMARKS=\
	aes/aes \
	backprop/backprop \
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

build:
	@( for b in $(BENCHMARKS); do $(MAKE) CFLAGS="$(CFLAGS)" -C $$b || exit ; done )

run:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b run || exit ; done )

clean:
	@( for b in $(BENCHMARKS); do $(MAKE) -C $$b clean || exit ; done )
