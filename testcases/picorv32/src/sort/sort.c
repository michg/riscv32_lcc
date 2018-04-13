/*
 *  Bubble sort
 */

int main()
{
  int a[10];
  int i, j, tmp;

  a[0] = 32;
  a[1] = 1;
  a[2] = 3;
  a[3] = 2;
  a[4] = 22;
  a[5] = 15;
  a[6] = 10;
  a[7] = 14;
  a[8] = 30;
  a[9] = 5;

  i = 0;
  while (i < 9) {
    j = 9;
    while (j >= i+1) {
      if (a[j] < a[j-1]) {
        tmp = a[j-1];
        a[j-1] = a[j];
        a[j] = tmp;
      }
      j--;
    }
    i++;
  }

  i = 0;
  while (i < 10) {
    puti(a[i]);
    putc('\n');
    i++;
  }

  return 0;
}
