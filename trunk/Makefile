# $Id: Makefile,v 1.15 2004-06-13 21:01:23 rousseau Exp $

# PC/SC Lite libraries and headers.
PCSC_CFLAGS ?= `pkg-config libpcsclite --cflags`
PCSC_LDLIBS ?= `pkg-config libpcsclite --libs`

# by default install in /usr/local
DESTDIR ?= /usr/local

VERSION=`echo \`pwd\` | sed s/.*tools-//`
CFLAGS=-Wall -O2 -g -DVERSION=\"$(VERSION)\" $(PCSC_CFLAGS)
LDLIBS=$(PCSC_LDLIBS)
# On xBSD systems use
#LDLIBS=-lc_r $(PCSC_LDLIBS)
# on MacOSX
#CFLAGS=-Wall -O2 -DVERSION=\"$(VERSION)\" -framework PCSC
#LDLIBS=-framework PCSC

BIN=pcsc_scan
BIN_SCRIPT=ATR_analysis gscriptor scriptor
MAN=pcsc_scan.1.gz gscriptor.1p.gz scriptor.1p.gz ATR_analysis.1p.gz

all: $(BIN) $(MAN)

pcsc_scan: pcsc_scan.o

install: all
	install -d $(DESTDIR)/bin/
	install $(BIN) $(DESTDIR)/bin/

	install $(BIN_SCRIPT) $(DESTDIR)/bin/

	install -d $(DESTDIR)/pcsc
	install -m 644 smartcard_list.txt $(DESTDIR)/pcsc

	install -d $(DESTDIR)/share/man/man1/
	install -m 644 $(MAN) $(DESTDIR)/share/man/man1/

clean:
	rm -f pcsc_scan.o $(BIN) $(MAN)

%.1.gz: %.1
	gzip --best $^ --to-stdout > $@

%.1p.gz: %.1p
	gzip --best $^ --to-stdout > $@

.PHONY: clean all install

