int sum(int x, int y, int z, int a, int b, int c)
{
  int result;

  result = x + y + z + a + b + c;
  return result;
}

int main()
{
  int x, y, z, a, b, c;
  int res;
  int z1, s;

  x = 5;
  y = 20;
  z = 30;
  a = 35;
  b = 40;
  c = 45;

  if (x != y) {
    z1 = sum(x, y, z, a, b, c);
    res = z1;
  } else {
    s = sum(x, 33, z, 22, 33, 44);
    res = s;
  }

  puti(res);
  putc('\n');

  return res;
}
