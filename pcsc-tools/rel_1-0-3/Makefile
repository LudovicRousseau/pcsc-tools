
# $Id: Makefile,v 1.6 2001-11-08 08:44:10 rousseau Exp $

CC=gcc
VERSION=`echo \`pwd\` | sed s/.*tools-//`
CFLAGS=-Wall -O2 -DVERSION=\"$(VERSION)\"
LDLIBS=-lpcsclite -lpthread

BIN=pcsc_scan
MAN=pcsc_scan.1.gz gscriptor.1p.gz scriptor.1p.gz

all: $(BIN) $(MAN)

pcsc_scan: pcsc_scan.o

install: all
	install --directory $(DESTDIR)/usr/bin/
	install $(BIN) $(DESTDIR)/usr/bin/

	install gscriptor scriptor $(DESTDIR)/usr/bin/

	install --directory $(DESTDIR)/usr/share/man/man1/
	install --mode 644 $(MAN) $(DESTDIR)/usr/share/man/man1/

clean:
	rm -f pcsc_scan.o $(BIN) $(MAN)

%.1.gz: %.1
	gzip --best $^ --to-stdout > $@

%.1p.gz: %.1p
	gzip --best $^ --to-stdout > $@

.PHONY: clean all install

