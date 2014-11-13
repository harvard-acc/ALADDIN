md: md.c md.h ../../common/harness.c
	$(CC) $(CFLAGS) -o md md.c ../../common/harness.c

run: md input.data check.data
	./md input.data check.data

hls: md.c md.h
	vivado_hls hls.tcl

clean:
	rm -f md
