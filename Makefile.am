rcdir = $(sysconfdir)/xdg/$(PACKAGE_NAME)

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc src
dist_doc_DATA = AUTHORS ChangeLog NEWS README
EXTRA_DIST = autogen.sh README.source

dist_rc_DATA = tint2rc

if INSTALL_EXAMPLES
examplesdir = $(docdir)/examples
dist_examples_DATA = tintrc01 tintrc02 tintrc03 tintrc04 tintrc05 tintrc06
else
examplesdir =
dist_examples_DATA =
endif

DISTCLEANFILES = configure depcomp config.guess config.sub config.h.in config.h.in~ ltmain.sh missing aclocal.m4 install-sh Makefile.in INSTALL

dist-hook:
	find $(distdir)/ -type d -name '.svn' -exec rm -rf {} ';'

distclean-local: distclean-local-check

distclean-local-check:
	-rm -rf autom4te.cache m4
