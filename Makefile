# PC/SC Lite libraries and headers.
PCSC_CFLAGS ?= $(shell pkg-config libpcsclite --cflags)
PCSC_LDLIBS ?= $(shell pkg-config libpcsclite --libs)

# by default install in /usr/local
DESTDIR ?= /usr/local

VERSION := $(shell pwd | sed s/.*tools-//)
CFLAGS := $(EXT_CFLAGS) -DVERSION=\"$(VERSION)\" $(PCSC_CFLAGS)
LDLIBS := $(PCSC_LDLIBS)
# On xBSD systems use
#LDLIBS = -lc_r $(PCSC_LDLIBS)
# on MacOSX
#CFLAGS = -Wall -O2 -DVERSION=\"$(VERSION)\"
#LDLIBS = -framework PCSC

INSTALL_PREFIX=/usr
INSTALL_BINDIR=$(INSTALL_PREFIX)/bin
INSTALL_DATADIR=$(INSTALL_PREFIX)/share
INSTALL_MANDIR=$(INSTALL_DATADIR)/man

BIN = pcsc_scan
BIN_SCRIPT = ATR_analysis gscriptor scriptor
MAN = pcsc_scan.1.gz gscriptor.1p.gz scriptor.1p.gz ATR_analysis.1p.gz

all: $(BIN) $(MAN)

pcsc_scan: pcsc_scan.o

install: all
	install -d $(DESTDIR)/$(INSTALL_BINDIR)/
	install $(BIN) $(DESTDIR)/$(INSTALL_BINDIR)/

	install $(BIN_SCRIPT) $(DESTDIR)/$(INSTALL_BINDIR)/

	install -d $(DESTDIR)/$(INSTALL_DATADIR)/pcsc
	install -m 644 smartcard_list.txt $(DESTDIR)/$(INSTALL_DATADIR)/pcsc

	install -d $(DESTDIR)/$(INSTALL_MANDIR)/man1/
	install -m 644 $(MAN) $(DESTDIR)/$(INSTALL_MANDIR)/man1/

clean:
	rm -f pcsc_scan.o $(BIN) $(MAN)

%.1.gz: %.1
	gzip --best $^ --to-stdout > $@

%.1p.gz: %.1p
	gzip --best $^ --to-stdout > $@

.PHONY: clean all install Changelog

Changelog:
	git log --stat --decorate=short > $@
