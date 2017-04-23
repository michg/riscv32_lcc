typedef unsigned char byte;
typedef struct uart{
unsigned int dr;
unsigned int sr;
unsigned int ack;
} UART;


UART* uart1 = (UART*)(0x20000000);
int i=0;
short *p;

void putcmon(byte c)
{
    uart1->dr = c;
}

void printmon(char* txt)
{
    while (*txt)
    {
        putcmon(*txt);
        txt++;
    }
}

void printintmon(unsigned int i)
{
    int b;
    unsigned int c;
    printmon("0x");
    for (b = 28; b >= 0; b = b - 4)
    {
        c = (i >> b) & 0xF;
        if (c < 10)
        {
            putcmon( 48 + c );
        }
        else
        {
            putcmon( 65 - 10 + c );
        }
    }
    putcmon(10); // Newline!
}

unsigned int find_max(unsigned int *arr, unsigned int size)
{
  unsigned int res = 0, i;

  for (i = 0; i < size; i++)
    if (arr[i] > res)
      res = arr[i];

  return res;
}

void main()
{

  unsigned int max_elem;
  unsigned int arr[5];

  arr[0] = 5;
  arr[1] = 54;
  arr[2] = 1;
  arr[3] = 94;
  arr[4] = 12;

  max_elem = find_max(arr, 5);

  printintmon(max_elem);
  putcmon('\n');

  while(1) {}
}