# Makefile for various po files.

srcdir = .
libdir = ../lib

MSGMERGE  = msgmerge
MSGFMT    = msgfmt
XGETTEXT  = xgettext
CATOBJEXT = .po
PERL      = perl

include PACKAGE

MO_FILES = $(addsuffix .gmo, $(LINGUAS))

TD = $(strip $(TEXTDOMAIN))

default: help

all: $(TD).pot update-po update-mo install

help:
	@echo "Available targets:"
	@echo "  pot                       - remake master catalog"
	@echo "  update-po                 - merge po files"
	@echo "  update-mo                 - regenerate mo files"
	@echo "  install                   - install mo files"
	@echo "  all                       - all of the above"

POTFILES = $(srcdir)/POTFILES \
	$(shell cat $(srcdir)/POTFILES) 

pot: $(TD).pot 

clean:
	rm -f *~ *.bak *.gmo

$(TD).pot: $(POTFILES)
	$(XGETTEXT) --output=$(srcdir)/$(TD).pox --from-code=utf-8 \
		--add-comments=TRANSLATORS: --files-from=$(srcdir)/POTFILES \
		--copyright-holder="$(COPYRIGHT_HOLDER)" \
		--msgid-bugs-address="$(MSGID_BUGS_ADDRESS)" \
                `$(PERL) -MLocale::TextDomain \
                -e "print Locale::TextDomain->options"` && \
	rm -f $@ && mv $(TD).pox $@

install: $(MO_FILES)
	cd $(srcdir); \
	targetdir='$(libdir)/LocaleData'; \
	languages='$(LINGUAS)'; \
	for lang in $$languages; do \
		mkdir -p "$$targetdir/$$lang/LC_MESSAGES" || exit 1; \
		dest="$$targetdir/$$lang/LC_MESSAGES/$(TD).mo"; \
		cat="$$lang.gmo"; \
		echo "installing $$cat as $$dest"; \
		cp -f $$cat $$dest && chmod 644 $$dest || exit 1; \
	done

update-mo: $(MO_FILES)

update-po:
	$(MAKE) $(TD).pot
	cd $(srcdir); \
        catalogs='$(LINGUAS)'; \
        for cat in $$catalogs; do \
          cat=`basename $$cat`; \
          lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
          mv $$lang.po $$lang.old.po; \
          echo "$$lang:"; \
          if $(MSGMERGE) $$lang.old.po $(TD).pot -o $$lang.po; then \
            rm -f $$lang.old.po; \
          else \
            echo "msgmerge for $$cat failed!"; \
            rm -f $$lang.po; \
            mv $$lang.old.po $$lang.po; \
          fi; \
        done

.SUFFIXES:
.SUFFIXES: .po .gmo

.po.gmo:
	$(MSGFMT) --check --statistics --verbose -o $@ $<

