LANGUAGES = \
		ja

MOFILES = \
		ja.mo

PACKAGE = gtktwitter
SHELL = /bin/sh

# The OLD_PO_FILE_INPUT and OLD_PO_FILE_OUTPUT are for the new GNU gettext
# tools 0.10.37, which use a slightly different .po file format that is not
# compatible with Solaris (and old gettext implementations) unless these are
# set.  gettext 0.10.36 will not work!
MSGFMT = OLD_PO_FILE_INPUT=yes msgfmt -v
XGETTEXT = OLD_PO_FILE_INPUT=yes OLD_PO_FILE_OUTPUT=yes xgettext
MSGMERGE = OLD_PO_FILE_INPUT=yes OLD_PO_FILE_OUTPUT=yes msgmerge

.SUFFIXES:
.SUFFIXES: .po .mo .pot .ck
.PHONY: all install uninstall clean distclean $(LANGUAGES)

.po.mo:
	$(MSGFMT) -o $@ $<

all: $(MOFILES)

install: $(MOFILES)
	for lang in $(LANGUAGES); do \
	  dir=$(LOCALE_DIR)/$$lang/; \
	  if test ! -x "$$dir"; then \
	    mkdir $$dir; chmod 755 $$dir; \
	  fi; \
	  dir=$(LOCALE_DIR)/$$lang/LC_MESSAGES; \
	  if test ! -x "$$dir"; then \
	    mkdir $$dir; chmod 755 $$dir; \
	  fi; \
	  if test -r $$lang.mo; then \
	    $(INSTALL_DATA) $$lang.mo $$dir/$(PACKAGE).mo; \
	    chmod $(FILEMOD) $$dir/$(PACKAGE).mo; \
	  fi; \
	done

uninstall:
	for cat in $(MOFILES); do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed 's/\$(CATOBJEXT)$$//'`; \
	  rm -f $(LOCALE_DIR)/$$lang/LC_MESSAGES/$(PACKAGE).mo; \
	done

clean: 
	rm -f core core.* *.old.po *.mo *.pot sjiscorr

distclean: clean

$(PACKAGE).pot: ../*.c
	cd ..; $(XGETTEXT) --default-domain=$(PACKAGE) \
		--add-comments --keyword=_ --keyword=N_ \
		*.c
	mv -f ../$(PACKAGE).po $(PACKAGE).pot

$(LANGUAGES):
	@$(MAKE) $(PACKAGE).pot
	if test ! -f $@.po.orig; then cp $@.po $@.po.orig; fi
	mv $@.po $@.po.old
	if $(MSGMERGE) $@.po.old $(PACKAGE).pot -o $@.po; then \
	    rm -f $@.po.old; \
	else \
	    echo "msgmerge for $@.po failed!"; mv $@.po.old $@.po; \
	fi
