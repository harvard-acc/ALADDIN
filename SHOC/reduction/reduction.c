#include "reduction.h"

int reduction(int *in)
{
  int i = 0;
  int sum = 0;

  for (i = 0; i < size; i++)
    sum += in[i];
  
  return sum;
}


int main()
{
  int *in;
  in = (int *) malloc (sizeof(int) * size );

  int i;
  int max = 2147483646;
  int min = 0;
  srand(8650341L);
  for (i = 0; i < size; i++)
  {
    in[i] = (int)(min + rand() * 1.0 * (max - min) / (RAND_MAX ));
  }

  int sum = reduction(&in[0]);
  printf("sum: %d\n", sum);
  return 0;
}
