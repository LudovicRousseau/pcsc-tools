
# $Id: Makefile,v 1.12 2004-01-08 09:23:19 rousseau Exp $

# The root of the include and lib directory where PCSC Lite is installed.
PCSCBASE ?= /usr/local

# by default install in /usr/local
DESTDIR ?= /usr/local

VERSION=`echo \`pwd\` | sed s/.*tools-//`
CFLAGS=-Wall -O2 -DVERSION=\"$(VERSION)\" -I$(PCSCBASE)/include
LDLIBS=-lpcsclite -lpthread -L$(PCSCBASE)/lib
# On xBSD systems use
#LDLIBS=-lpcsclite -lc_r -L$(PCSCBASE)/lib
# on MacOSX
#CFLAGS=-Wall -O2 -DVERSION=\"$(VERSION)\" -framework PCSC
#LDLIBS=-framework PCSC

BIN=pcsc_scan
BIN_SCRIPT=ATR_analysis gscriptor scriptor
MAN=pcsc_scan.1.gz gscriptor.1p.gz scriptor.1p.gz ATR_analysis.1p.gz

all: $(BIN) $(MAN)

pcsc_scan: pcsc_scan.o

install: all
	install --directory $(DESTDIR)/bin/
	install $(BIN) $(DESTDIR)/bin/

	install $(BIN_SCRIPT) $(DESTDIR)/bin/

	install --directory $(DESTDIR)/pcsc
	install --mode 644 smartcard_list.txt $(DESTDIR)/pcsc

	install --directory $(DESTDIR)/share/man/man1/
	install --mode 644 $(MAN) $(DESTDIR)/share/man/man1/

clean:
	rm -f pcsc_scan.o $(BIN) $(MAN)

%.1.gz: %.1
	gzip --best $^ --to-stdout > $@

%.1p.gz: %.1p
	gzip --best $^ --to-stdout > $@

.PHONY: clean all install

