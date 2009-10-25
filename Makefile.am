ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc sample src
dist_doc_DATA = AUTHORS ChangeLog NEWS README
EXTRA_DIST = autogen.sh README.source

dist_pkgdata_DATA = default_icon.png

DISTCLEANFILES += configure depcomp config.guess config.sub config.h.in config.h.in~ ltmain.sh missing aclocal.m4 install-sh INSTALL

dist-hook:
	find $(distdir)/ -type d -name '.svn' -exec rm -rf {} ';'

distclean-local: distclean-local-check

distclean-local-check:
	-rm -rf autom4te.cache m4
