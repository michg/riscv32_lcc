#include <float.h>
int ee_printf(const char *fmt, ...);

float x = 3.1415;
float y = 2.4563;
float z;

int main(void) {
  z = x + y;
  ee_printf("Result:%f",z);
  return 0;
}
