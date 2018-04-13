#include <stdarg.h>
void printargs(int arg1, ...)
{
  va_list ap;
  int i;

  va_start(ap, arg1);
  for (i = arg1; i >= 0; i = va_arg(ap, int))
    puti(i);
  va_end(ap);
  putc('\n');
}


int main()
{
  printargs(5, 2, 14, 84, 97, 15, -1, 48, -1);
  printargs(84, 51, -1);
  return 0;
}
