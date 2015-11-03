/* Test the read-only mmap implementation.
 *
 * This test creates a file, write sequential binary data to it, and then mmaps
 * the file and verifies if the data loaded matches the expected values.
 *
 * Author: Sam Xi
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

int test_loads(char* correct_values, void* addr, int num_vals) {
  int num_failures = 0;
  char* mmap_data = (char*) addr;
  for (int i = 0; i < num_vals; i++) {
    if (mmap_data[i] != correct_values[i]) {
      fprintf(stdout, "FAILED: mmap_data[%d] = %d, should be %d\n",
              i, mmap_data[i], correct_values[i]);
      num_failures++;
    }
  }
  return num_failures;
}

int main() {
  // Best practice is to mmap at page size granularity.
  unsigned int NUM_PAGES = 20;
  size_t PAGE_SIZE = 4096;
  size_t buf_size = NUM_PAGES * PAGE_SIZE;
  char* buffer = (char*)malloc(buf_size*sizeof(char));

  // Load some data into our buffer.
  for (int i = 0; i < buf_size; i++) {
    buffer[i] = (char)i;
  }

  // Write binary data to file.
  FILE* stream = fopen("test.data", "wb");
  fwrite(buffer, sizeof(char), buf_size, stream);
  fclose(stream);

  // Mmap the file.
  int fd = open("test.data", O_RDONLY);
  void* addr = mmap(NULL, buf_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED) {
    perror("mmap failed");
    return -1;
  }

  int num_failures = test_loads(buffer, addr, buf_size);
  if (num_failures != 0) {
    fprintf(stdout, "Test failed with %d errors.", num_failures);
    return -1;
  }

  fprintf(stdout, "Test passed!\n");
  free(buffer);
  return 0;
}
