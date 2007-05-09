bin_PROGRAMS=gtktwitter
gtktwitter_SOURCES=gtktwitter.c
AM_CPPFLAGS=-DDATADIR=\"$(pkgdatadir)\" -DLOCALEDIR=\"$(datadir)/locale\"
INCLUDES=${GTK_CFLAGS}
gtktwitter_LDADD=${GTK_LIBS}
dist_pkgdata_DATA=data/loading.gif data/twitter.png data/reload.png data/post.png data/logo.png
EXTRA_DIST=gtktwitter.spec
