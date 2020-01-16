#include <stdio.h>

#define N 8

void test_store_buffer(int* input0, int* input1, int* result, int n) {
  int(*_result)[n] = (int(*)[n])result;

  loop:
  for (int i = 0; i < n; i++) {
    _result[input0[i]][i] = input0[i] + input1[i];
    _result[i][i] += _result[input0[i]][i];
  }
}

int main() {
  int input0[N];
  int input1[N];
  int result[N * N];
  for (int i = 0; i < N; i++) {
    input0[i] = i % N;
    input1[i] = i * i;
  }
  for (int i = 0; i < N * N; i++)
    result[i] = 0;
  test_store_buffer(input0, input1, result, N);
  for (int i = 0; i < N * N; i++)
    printf("%d,", result[i]);
  printf("\n");
  return 0;
}
