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

converted: $(CONVERTED)

# Convert ja.po to create ja.sjis.po.  Requires doubling backslashes in the
# second byte.  Don't depend on sjiscorr, it should only be compiled when
# ja.sjis.po is outdated.
ja.sjis.po: ja.po
	@$(MAKE) sjiscorr
	rm -f ja.sjis.po
	iconv -f euc-jp -t cp932 ja.po | ./sjiscorr > ja.sjis.po

sjiscorr: sjiscorr.c
	$(CC) -o sjiscorr sjiscorr.c

# Convert cs.po to create cs.cp1250.po.
cs.cp1250.po: cs.po
	rm -f cs.cp1250.po
	iconv -f iso-8859-2 -t cp1250 cs.po | \
		sed -e 's/charset=ISO-8859-2/charset=cp1250/' -e 's/# Original translations/# Generated from cs.po, DO NOT EDIT/' > cs.cp1250.po

# Convert pl.po to create pl.cp1250.po.
pl.cp1250.po: pl.po
	rm -f pl.cp1250.po
	iconv -f iso-8859-2 -t cp1250 pl.po | \
		sed -e 's/charset=ISO-8859-2/charset=cp1250/' -e 's/# Original translations/# Generated from pl.po, DO NOT EDIT/' > pl.cp1250.po

# Convert pl.po to create pl.UTF-8.po.
pl.UTF-8.po: pl.po
	rm -f pl.UTF-8.po
	iconv -f iso-8859-2 -t utf-8 pl.po | \
		sed -e 's/charset=ISO-8859-2/charset=utf-8/' -e 's/# Original translations/# Generated from pl.po, DO NOT EDIT/' > pl.UTF-8.po

# Convert sk.po to create sk.cp1250.po.
sk.cp1250.po: sk.po
	rm -f sk.cp1250.po
	iconv -f iso-8859-2 -t cp1250 sk.po | \
		sed -e 's/charset=ISO-8859-2/charset=cp1250/' -e 's/# Original translations/# Generated from sk.po, DO NOT EDIT/' > sk.cp1250.po

# Convert zh_CN.po to create zh_CN.cp936.po.
# set 'charset' to gbk to avoid that msfmt generates a warning
zh_CN.cp936.po: zh_CN.po
	rm -f zh_CN.cp936.po
	iconv -f gb2312 -t cp936 zh_CN.po | \
		sed -e 's/charset=gb2312/charset=gbk/' -e 's/# Original translations/# Generated from zh_CN.po, DO NOT EDIT/' > zh_CN.cp936.po

# Convert ru.po to create ru.cp1251.po.
ru.cp1251.po: ru.po
	rm -f ru.cp1251.po
	iconv -f utf-8 -t cp1251 ru.po | \
		sed -e 's/charset=utf-8/charset=cp1251/' -e 's/# Original translations/# Generated from ru.po, DO NOT EDIT/' > ru.cp1251.po

# Convert uk.po to create uk.cp1251.po.
uk.cp1251.po: uk.po
	rm -f uk.cp1251.po
	iconv -f koi8-u -t cp1251 uk.po | \
		sed -e 's/charset=koi8-u/charset=cp1251/' -e 's/# Original translations/# Generated from uk.po, DO NOT EDIT/' > uk.cp1251.po

clean: 
	rm -f core core.* *.old.po *.mo *.pot sjiscorr

distclean: clean

#
# NOTE: If you get an error for gvimext.cpp not found, you need to unpack the
# extra archive.
#
$(PACKAGE).pot: ../*.c
	cd ..; $(XGETTEXT) --default-domain=$(PACKAGE) \
		--add-comments --keyword=_ --keyword=N_ \
		*.c
	mv -f ../$(PACKAGE).po $(PACKAGE).pot

# Don't add a dependency here, we only want to update the .po files manually
$(LANGUAGES):
	@$(MAKE) $(PACKAGE).pot
	if test ! -f $@.po.orig; then cp $@.po $@.po.orig; fi
	mv $@.po $@.po.old
	if $(MSGMERGE) $@.po.old $(PACKAGE).pot -o $@.po; then \
	    rm -f $@.po.old; \
	else \
	    echo "msgmerge for $@.po failed!"; mv $@.po.old $@.po; \
	fi
