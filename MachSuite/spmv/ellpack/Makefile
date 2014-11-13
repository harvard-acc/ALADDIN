ellpack: ellpack.c ellpack.h ../../common/harness.c
	$(CC) $(CFLAGS) -o ellpack ellpack.c ../../common/harness.c

run: ellpack input.data check.data
	./ellpack input.data check.data

hls: ellpack.c ellpack.h
	vivado_hls hls.tcl

clean:
	rm -f ellpack
