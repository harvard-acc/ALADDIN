gemm: gemm.c gemm.h ../../common/harness.c
	$(CC) $(CFLAGS) -o gemm gemm.c ../../common/harness.c

hls: gemm.c gemm.h
	vivado_hls hls.tcl

run: gemm input.data check.data
	./gemm input.data check.data

clean:
	rm -f gemm
