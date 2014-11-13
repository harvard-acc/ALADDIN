needwun: needwun.c needwun.h ../../common/harness.c
	$(CC) $(CFLAGS) -o needwun needwun.c ../../common/harness.c

run: needwun input.data check.data
	./needwun input.data check.data

hls: needwun.c needwun.h
	vivado_hls hls.tcl

clean:
	rm -f needwun
