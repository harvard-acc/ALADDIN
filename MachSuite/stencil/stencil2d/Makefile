stencil: stencil.c stencil.h ../../common/harness.c
	$(CC) $(CFLAGS) -o stencil stencil.c ../../common/harness.c

hls: stencil.c stencil.h
	vivado_hls hls.tcl

run: stencil input.data check.data
	./stencil input.data check.data

clean:
	rm -f stencil
