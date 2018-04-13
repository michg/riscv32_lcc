void putc(char c)
{
    unsigned int* UART0=(unsigned int*)0x20000000;
    // Transmit char:
    *UART0 = c;
}



void puti(int i)
{
    int b=0;
    int c=0;

    for (b = 28; b >= 0; b = b - 4)
    {
        c = (i >> b) & 0xF;
        if (c < 10)
        {
            putc( 48 + c );
        }
        else
        {
            putc( 65 - 10 + c );
        }
    }

    putc(10); // Newline!
}

