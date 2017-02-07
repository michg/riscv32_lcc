/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include "shared_libc.h"

char *
gets(char *s)
{
	register int c;
	register char *sp;
#if	0
	static int warned;
	static char w[] = "warning: this program uses gets(), which is unsafe\r\n";


	if (!warned) {
		(void) fputs(w, stderr);
		warned = 1;
	}
#endif	/* 0 */

	sp = s;
	while ((c = getchar()) != '\n') {
		if (c == EOF) {
			if (sp == s) {
				return NULL;
			} else
				break;
		} else
			*sp++ = c;
	}
	*sp = 0;
	return s;
}
