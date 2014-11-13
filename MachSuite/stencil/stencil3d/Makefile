stencil3d: stencil3d.c stencil3d.h ../../common/harness.c
	$(CC) $(CFLAGS) -o stencil3d stencil3d.c ../../common/harness.c

hls: stencil3d.c stencil3d.h
	vivado_hls hls.tcl

run: stencil3d input.data check.data
	./stencil3d input.data check.data

clean:
	rm -f stencil3d
