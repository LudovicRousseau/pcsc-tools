
# $Id: Makefile,v 1.2 2001-10-16 07:31:35 rousseau Exp $

CC=gcc
VERSION=1.0.0
CFLAGS=-Wall -O2 -DVERSION=\"$(VERSION)\"
LDLIBS=-lpcsclite -lpthread

BIN=pcsc_scan

all: $(BIN)

install: all
	cp $(BIN) $(DESTDIR)/usr/bin

pcsc_scan: pcsc_scan.o

clean:
	rm -f pcsc_scan.o pcsc_scan

