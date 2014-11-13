merge: merge.c merge.h ../../common/harness.c
	$(CC) $(CFLAGS) -o merge merge.c ../../common/harness.c

run: merge input.data check.data
	./merge input.data check.data

hls: merge.c merge.h
	vivado_hls hls.tcl

clean:
	rm -f merge
