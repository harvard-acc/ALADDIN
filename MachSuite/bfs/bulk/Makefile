bulk: bulk.c bulk.h ../../common/harness.c
	$(CC) $(CFLAGS) -o bulk bulk.c ../../common/harness.c

run: bulk input.data check.data
	./bulk input.data check.data

hls: bulk.c bulk.h
	vivado_hls hls.tcl

clean:
	rm -f bulk
