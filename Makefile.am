bin_PROGRAMS = pcsc_scan
pcsc_scan_SOURCES = pcsc_scan.c pcsc_scan.1
pcsc_scan_CFLAGS = $(PCSC_CFLAGS)
pcsc_scan_LDADD = $(PCSC_LIBS)

#  install the man pages
man_MANS=pcsc_scan.1

EXTRA_DIST = Changelog

Changelog:
	git log --stat --decorate=short > $@
