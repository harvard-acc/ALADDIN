fft: fft.c fft.h ../../common/harness.c
	$(CC) $(CFLAGS) -o fft fft.c ../../common/harness.c -lm

run: fft input.data check.data
	./fft input.data check.data

hls: fft.c fft.h
	vivado_hls hls.tcl

clean:
	rm -f fft
