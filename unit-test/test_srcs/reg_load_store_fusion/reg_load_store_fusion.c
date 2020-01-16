#include <stdio.h>

void rmw_fusion(int* input0, int* input1, int* result) {
  outer:
  for (int i = 0; i < 10; i++) {
    inner:
    for (int j = 0; j < 8; j++) {
      result[i] += input0[j] * input1[j];
    }
  }
}

int main() {
  int result[10];
  int input0[8];
  int input1[8];
  for (int i = 0; i < 8; i++) {
    input0[i] = i * i + 1;
    input1[i] = i * i + 2;
  }
  rmw_fusion(input0, input1, result);
  printf("result: ");
  for (int i = 0; i < 10; i++)
    printf("%d,", result[i]);
  printf("\n");
  return 0;
}
