CC=gcc
CFLAGS=-Wall -O2 -DVERSION=\"none\"
LDLIBS=-lpcsclite -lpthread

BIN=pcsc_scan winscardtest bug

all: $(BIN)

install: all
	cp $(BIN) $(DESTDIR)/usr/bin

pcsc_scan: pcsc_scan.o

winscardtest: winscardtest.o

clean:
	rm -f winscardtest.o winscardtest
	rm -f pcsc_scan.o pcsc_scan

