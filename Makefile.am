SUBDIRS = po

PERL_BINS = ATR_analysis.in scriptor gscriptor
PERL_MANPAGES = ATR_analysis.1p.in scriptor.1p gscriptor.1p

bin_PROGRAMS = pcsc_scan
pcsc_scan_SOURCES = pcsc_scan.c pcsc_scan.1
pcsc_scan_CFLAGS = $(PCSC_CFLAGS)
pcsc_scan_LDADD = $(PCSC_LIBS)

bin_SCRIPTS = $(subst .in, , $(PERL_BINS))

pcsc_DATA = smartcard_list.txt
pcscdir = $(pcsc_dir)

#  install the man pages
man_MANS = pcsc_scan.1 $(subst .in, , $(PERL_MANPAGES))

EXTRA_DIST = \
	Changelog \
	LICENCE \
	$(PERL_BINS) \
	$(PERL_MANPAGES) \
	$(pcsc_DATA) \
	gscriptor.desktop \
	test.script

Changelog:
	git log --stat --decorate=short > $@
