struct Point {
  int x;
  int y;
} p;

int main()
{
  int sum;
  p.x = 123;
  p.y = 456;

  sum = p.y + p.x;
  puti(sum);
  putc('\n');
  return 0;
}
