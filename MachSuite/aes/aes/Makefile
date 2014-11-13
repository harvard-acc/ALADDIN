aes: aes.c aes.h ../../common/harness.c
	$(CC) $(CFLAGS) -o aes aes.c ../../common/harness.c

run: aes input.data check.data
	./aes input.data check.data

hls: aes.c aes.h
	vivado_hls hls.tcl

clean:
	rm -f aes
