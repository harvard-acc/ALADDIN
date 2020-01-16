#include <stdio.h>

#define N 8

void test_independent(
    int* input0, int* input1, int* result, int rows, int cols) {
  int(*_result)[rows] = (int(*)[rows])result;

  outer:
  for (int i = 0; i < rows; i++) {
    inner:
    for (int j = 0; j < cols; j++) {
      _result[i][j] = input0[j] + input1[j];
    }
  }
}

void test_indirect(int* input0, int* input1, int* result, int rows, int cols) {
  int(*_result)[rows] = (int(*)[rows])result;

  outer:
  for (int i = 0; i < rows; i++) {
    inner:
    for (int j = 0; j < cols; j++) {
      _result[input0[j]][j] = input0[j] + input1[j];
    }
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
  test_independent(input0, input1, result, N, N);
  test_indirect(input0, input1, result, N, N);
  for (int i = 0; i < N * N; i++)
    printf("%d,", result[i]);
  printf("\n");
  return 0;
}
