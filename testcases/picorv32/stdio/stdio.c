#include <stdio.h>

float x = 3.1415;
float y = 2.4563;
float z;

int main(void) {
  z = x - y;
  printf("x - y = %2.4f\n",z);
  z = x + y;
  printf("x + y = %2.4f\n",z);
  z = x * y;
  printf("x * y = %2.4f\n",z);
  z = x / y;
  printf("x / y = %2.4f\n",z);
  return 0;
}
