
# $Id: Makefile,v 1.3 2001-10-22 09:07:21 rousseau Exp $

CC=gcc
VERSION=`echo \`pwd\` | sed s/.*tools-//`
CFLAGS=-Wall -O2 -DVERSION=\"$(VERSION)\"
LDLIBS=-lpcsclite -lpthread

BIN=pcsc_scan

all: $(BIN)

install: all
	cp $(BIN) $(DESTDIR)/usr/bin

pcsc_scan: pcsc_scan.o

clean:
	rm -f pcsc_scan.o pcsc_scan

