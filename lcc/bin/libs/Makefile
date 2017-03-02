#
# Makefile for building the libraries
#


DIRS =ctype eeprintf errno io math softfloat stdio stdlib string

.PHONY:		all install clean

all:
		for i in $(DIRS) ; do \
		  $(MAKE) -r -C $$i all ; \
		done


clean:
		for i in $(DIRS) ; do \
		  $(MAKE) -r -C $$i clean ; \
		done
		rm -f *~
