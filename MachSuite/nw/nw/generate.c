#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "needwun.h"
// Fake benchmark function to satisfy the extern

void needwun(char SEQA[N], char SEQB[M], char alignedA[sum_size], char alignedB[sum_size],
             int A[dyn_size], char ptr[dyn_size]) {}
void generate_binary()
{
  struct bench_args_t data;
  char seqA[N] = "tcgacgaaataggatgacagcacgttctcgtattagagggccgcggtacaaaccaaatgctgcggcgtacagggcacggggcgctgttcgggagatcgggggaatcgtggcgtgggtgattcgccggc";

  char seqB[M] = "ttcgagggcgcgtgtcgcggtccatcgacatgcccggtcggtgggacgtgggcgcctgatatagaggaatgcgattggaaggtcggacgggtcggcgagttgggcccggtgaatctgccatggtcgat";
  //char seqB[M] = "acgcccatacgatttatagcttcacgcgattaacgttctgttgataattccgtaaatcctatctctgggcgcatgcgagaagctcatagttaggtcaacatacggccactcgctgccttagaatttccgttagtcgtataactaaccgactcgatgtccttccgccgcccctagtgactgtactcgaatgcctctctcgaacaccccccggagcaggccaggtttggcagttacgtggttgcacgttcatgggctc";
  char *ptr;
  int status, i, fd, written=0;

  // Fill data structure
  srandom(1);
  memcpy(data.seqA, seqA, N);
  memcpy(data.seqB, seqB, M);
  memset(&data.alignedA, 0, (N+M));
  memset(&data.alignedB, 0, (N+M));
  memset(&data.A, 0, dyn_size);
  memset(&data.ptr, 0, dyn_size);
  int j = 0;
  int mismatch = -1;
  for (j = 0;  j < N1; j++)
    data.A[j] = j * mismatch;
  for (j = 0;  j < M1; j++)
    data.A[j*N1] = j * mismatch;
  // Open and write
  fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  assert( fd>0 && "Couldn't open input data file" );

  ptr = (char *) &data;
  while( written<sizeof(data) ) {
    status = write( fd, ptr, sizeof(data)-written );
    assert( status>=0 && "Couldn't write input data file" );
    written += status;
  }
}

int main(int argc, char **argv)
{
  generate_binary();
  return 0;
}
