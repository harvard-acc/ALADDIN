crs: crs.c crs.h ../../common/harness.c
	$(CC) $(CFLAGS) -o crs crs.c ../../common/harness.c

run: crs input.data check.data
	./crs input.data check.data

hls: crs.c crs.h	
	vivado_hls hls.tcl

clean:
	rm -f crs
