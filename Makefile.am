bin_PROGRAMS=gtktwitter
gtktwitter_SOURCES=gtktwitter.c
AM_CPPFLAGS=-DDATA_DIR=\"$(pkgdatadir)\" -DLOCALE_DIR=\"$(datadir)/locale\"
INCLUDES=${GTK_CFLAGS}
gtktwitter_LDADD=${GTK_LIBS}
dist_pkgdata_DATA=data/twitter.png data/loading.gif data/reload.png data/config.png data/post.png data/home.png data/logo.png
EXTRA_DIST=gtktwitter.spec
