kmp: kmp.c kmp.h ../../common/harness.c
	$(CC) $(CFLAGS) -o kmp kmp.c ../../common/harness.c

run: kmp input.data check.data
	./kmp input.data check.data

hls: kmp.c kmp.h
	vivado_hls hls.tcl

clean:
	rm -f kmp
