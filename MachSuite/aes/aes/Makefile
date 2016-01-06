aes: aes.c aes.h ../../common/harness.c
	$(CC) $(CFLAGS) -o aes $(SRCS) $(LFLAGS)

run: aes input.data check.data
	./aes input.data check.data

hls: aes.c aes.h
	vivado_hls hls.tcl

clean:
	rm -f aes

include ../../common/Makefile.gem5

# For the tracer makefile
SRCS = aes.c ../../common/harness.c
ACCEL_NAME = aes
TEST_BIN = $(ACCEL_NAME)
export TRACE_OUTPUT_DIR=$(ACCEL_NAME)
ifndef WORKLOAD
  export WORKLOAD=gf_alog,gf_log,gf_mulinv,rj_sbox_rj_xtime,aes_subBytes,aes_addRoundKey,aes_addRoundKey_cpy,aes_shiftRows,aes_mixColumns,aes_expandEncKey,aes256_encrypt_ecb
endif
include ../../common/Makefile.tracer
