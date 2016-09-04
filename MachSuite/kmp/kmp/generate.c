#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "kmp.h"

int main(int argc, char **argv)
{
  struct bench_args_t data;
  int status, fd, nbytes;

  // Load string file
  fd = open("TR.txt", O_RDONLY);
  assert( fd>=0 && "couldn't open text file" );
  nbytes = 0;
  do {
    status = read(fd, data.input, STRING_SIZE-nbytes);
    assert(status>=0 && "couldn't read from text file");
    nbytes+=status;
  } while( nbytes<STRING_SIZE );
  close(fd);
  memcpy(data.pattern, "bull", PATTERN_SIZE);

  // Open and write
  fd = open("input.data", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  assert( fd>0 && "Couldn't open input data file" );

  data_to_input(fd, (void *)(&data));

  return 0;
}
