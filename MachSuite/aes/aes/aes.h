/*
*   Byte-oriented AES-256 implementation.
*   All lookup tables replaced with 'on the fly' calculations.
*
*   Copyright (c) 2007-2009 Ilya O. Levin, http://www.literatecode.com
*   Other contributors: Hal Finney
*   Copyright (c) 2014, the President and Fellows of Harvard College
*
*   Permission to use, copy, modify, and distribute this software for any
*   purpose with or without fee is hereby granted, provided that the above
*   copyright notice and this permission notice appear in all copies.
*
*   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
*   WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
*   MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
*   ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
*   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
*   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
*   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#ifndef uint8_t
#define uint8_t unsigned char
#endif

#ifdef GEM5_HARNESS
#include "gem5/aladdin_sys_connection.h"
#include "gem5/aladdin_sys_constants.h"
#endif

typedef struct {
  uint8_t key[32];
  uint8_t enckey[32];
  uint8_t deckey[32];
} aes256_context;

void aes256_encrypt_ecb(aes256_context* ctx, uint8_t k[32], uint8_t buf[16]);

extern const uint8_t sbox[256];
extern uint8_t rcon;

////////////////////////////////////////////////////////////////////////////////
// Test harness interface code.

struct bench_args_t {
  aes256_context ctx;
  uint8_t k[32];
  uint8_t buf[16];
};
int INPUT_SIZE = sizeof(struct bench_args_t);

void run_benchmark(void* vargs) {
  struct bench_args_t* args = (struct bench_args_t*)vargs;

#ifdef GEM5_HARNESS
  mapArrayToAccelerator(
      MACHSUITE_AES_AES, "ctx", (void*)&args->ctx, sizeof(args->ctx));
  mapArrayToAccelerator(
      MACHSUITE_AES_AES, "k", (void*)&(args->k[0]), sizeof(args->k));
  mapArrayToAccelerator(
      MACHSUITE_AES_AES, "buf", (void*)&(args->buf[0]), sizeof(args->buf));
  mapArrayToAccelerator(
      MACHSUITE_AES_AES, "sbox", (void*)&(sbox[0]), sizeof(sbox));
  mapArrayToAccelerator(MACHSUITE_AES_AES, "rcon", (void*)&rcon, sizeof(rcon));
  int finish_flag = NOT_COMPLETED;
  invokeAccelerator(MACHSUITE_AES_AES, &finish_flag);
  while (finish_flag == NOT_COMPLETED)
    ;
#else
  aes256_encrypt_ecb(&(args->ctx), args->k, args->buf);
#endif
}

////////////////////////////////////////////////////////////////////////////////
