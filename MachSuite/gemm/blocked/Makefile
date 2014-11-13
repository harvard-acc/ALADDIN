bbgemm: bbgemm.c bbgemm.h ../../common/harness.c
	$(CC) $(CFLAGS) -o bbgemm bbgemm.c ../../common/harness.c

hls: bbgemm.c bbgemm.h
	vivado_hls hls.tcl

run: bbgemm input.data check.data
	./bbgemm input.data check.data

clean:
	rm -f bbgemm
