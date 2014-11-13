queue: queue.c queue.h ../../common/harness.c
	$(CC) $(CFLAGS) -o queue queue.c ../../common/harness.c

run: queue input.data check.data
	./queue input.data check.data

hls: queue.c queue.h
	vivado_hls hls.tcl

clean:
	rm -f queue
