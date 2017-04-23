#
# Makefile for Libs

LCCDIR =../..
CC = ${LCCDIR}/lcc
AS = ${LCCDIR}/../../binutils/bin/as
AR = ${LCCDIR}/../../binutils/bin/ar

CFLAGS = -Wo-lccdir=${LCCDIR} -S -D__TCS_V2__ -D__MIPSEL__ -w -v


CSRCS = $(wildcard *.c)
ASRCS = $(patsubst %.c,%.s,$(CSRCS))
OSRCS = $(patsubst %.c,%.o,$(CSRCS))
BIN = ${CURDIR}.lib

.PHONY:		all clean

all:		$(BIN)

%.s:%.c
	$(CC) $(CFLAGS) $<

%.o:%.s
	$(AS) -o $@ $<

$(BIN):$(OSRCS)
	$(AR) -rsv $@ $^

clean:
		rm -f *~ $(OSRCS) $(ASRCS) $(BIN)
