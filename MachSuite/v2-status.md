# v2.0 release status

## Updated benchmarks

Benchmark | Kernel | Generator | Checker
--------- | ------ | --------- | -------
`backprop` | | |
`viterbi` | | |

## I/O and checking
`Makefile`:
 - Add `support.c` to everything
 - Add a `generate` target

input->data:
 - Use `support.h` functions to read an input fd into the data struct

data->output:
 - Use `support.h` functions

header:
 - new prototypes
 - move wrapper to `support.c`

checker summary key:
 - (A) ASCII array needed
 -  (C) string comparison
 - (1) single number comparison
 - (+) additional work needed

Benchmark | make | in-data | data-out | hdr | checker (status) | checker (summary) | checker (description)
--------- | ---- | ------- | -------- | --- | ---------------- | ----------------- | ---------------------
`aes/aes` |X|X|X|X|X|C|string compare
`backprop/backprop` |X| | | | |+?|(more work needed)
`bfs/bulk` |X|X|X|X|X|A|horizon counts
`bfs/queue` |X|X|X|X|X|A|horizon counts
`fft/strided` |X|X|X|X|X|A|compare array ±eps
`fft/transpose` |X|X|X|X|X|A|compare array ±eps
`gemm/ncubed` |X|X|X|X|X|A+|compare matrix ±eps; change data type to float
`gemm/blocked` |X|X|X|X|X|A+|compare matrix ±eps; change data type to float
`kmp/kmp` |X|X|X|X|X|1|match count
`md/knn` |X|X|X|X|X|A|compare positions ±eps
`md/grid` |X|X|X|X|X|A|compare positions ±eps
`nw/nw` |X|X|X|X|X|C|Compare aligned sequences
`sort/merge` |X|X|X|X|X|1+|Check sortedness and sum
`sort/radix` |X|X|X|X|X|1+|Check sortedness and sum
`spmv/crs` |X|X|X|X|X|A|Compare vector ±eps
`spmv/ellpack` |X|X|X|X|X|A|Compare vector ±eps
`stencil/stencil2d` |X|X|X|X|X|A|Compare matrix
`stencil/stencil3d` |X|X|X|X|X|A|Compare matrix
`viterbi/viterbi` |X|X|X|X|X|+?|(more work needed)

## Large Inputs
Benchmark | Generator | header | kernel | size tuned | runtime tuned
--------- | --------- | ------ | ------ | ---------- | -------------
`aes/aes` | | | | |
`backprop/backprop` | | | | |
`bfs/bulk` | | | | |
`bfs/queue` | | | | |
`fft/strided` | | | | |
`fft/transpose` | | | | |
`gemm/ncubed` | | | | |
`gemm/blocked` | | | | |
`kmp/kmp` | | | | |
`md/knn` | | | | |
`md/grid` | | | | |
`nw/nw` | | | | |
`sort/merge` | | | | |
`sort/radix` | | | | |
`spmv/crs` | | | | |
`spmv/ellpack` | | | | |
`stencil/stencil2d` | | | | |
`stencil/stencil3d` | | | | |
`viterbi/viterbi` | | | | |
