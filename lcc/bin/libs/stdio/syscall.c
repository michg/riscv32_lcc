/* system calls */

#define	CHAR_IN			(*(volatile char *) 0xb0c20000)
#define	CHAR_OUT		(*(volatile char *) 0x20000000)

int _lseek()
{
	return 0;
}

int lseek()
{
	return 0;
}

int _fstat()
{
	return -1;
}

void _exit(int status)
{
	//if(status)
	//	_bup_happy();
	//else
	//	_bup_sad();
}

int _isatty()
{
	return 1;
}

int _read(int fildes, void *buf, int nbyte)
{
	char *p = (char *) buf, c = 0;
	int i;

	for(i = 0; i < nbyte && c != '\n'; i++) 
		*p++ = c = CHAR_IN;

	return i;
}

int read(int fildes, void *buf, int nbyte)
{
	return _read(fildes, buf, nbyte);
}

int _write(int fildes, const void *buf, int nbyte)
{
	char *p = (char *) buf;
	int i;

	for(i = 0; i < nbyte; i++)
		CHAR_OUT = *p++;

	return nbyte;
}

int write(int fildes, const void *buf, int nbyte)
{
	return _write(fildes, buf, nbyte);
}

int _close()
{
	return 0;
}

int _open()
{
	return 0;
}

void serial_out(char *s)
{
	while(*s)
		CHAR_OUT = *s++;
}
