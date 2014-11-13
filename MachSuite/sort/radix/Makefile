radix: radix.c radix.h ../../common/harness.c
	$(CC) $(CFLAGS) -o radix radix.c ../../common/harness.c

run: radix input.data check.data
	./radix input.data check.data

hls: radix.c radix.h
	vivado_hls hls.tcl

clean:
	rm -f radix
