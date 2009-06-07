#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([tint2], [0.7_beta2], [http://code.google.com/p/tint2/issues])

LT_INIT
AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/tint.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo])
AC_SUBST(PANGOCAIRO_CFLAGS)
AC_SUBST(PANGOCAIRO_LIBS)

PKG_CHECK_MODULES([PANGO], [pango])
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LIBS)

PKG_CHECK_MODULES([CAIRO], [cairo])
AC_SUBST(CAIRO_CFLAGS)
AC_SUBST(CAIRO_LIBS)

PKG_CHECK_MODULES([GLIB2], [glib-2.0])
AC_SUBST(GLIB2_CFLAGS)
AC_SUBST(GLIB2_LIBS)

PKG_CHECK_MODULES([GOBJECT2], [gobject-2.0])
AC_SUBST(GOBJECT2_CFLAGS)
AC_SUBST(GOBJECT2_LIBS)

PKG_CHECK_MODULES([X11], [x11])
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

PKG_CHECK_MODULES([XINERAMA], [xinerama])
AC_SUBST(XINERAMA_CFLAGS)
AC_SUBST(XINERAMA_LIBS)

PKG_CHECK_MODULES([IMLIB2], [imlib2])
AC_SUBST(IMLIB2_CFLAGS)
AC_SUBST(IMLIB2_LIBS)

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT16_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset select setlocale strcasecmp strchr strdup])

AC_ARG_ENABLE([examples],
			  [AS_HELP_STRING([--enable-examples], [Install additional $(PACKAGE_NAME)rc examples])],
			  [case "${enableval}" in
			   yes) examples=true ;;
			   no)  examples=false ;;
			   *) AC_MSG_ERROR([bad value ${enableval} for --enable-examples]) ;;
		   esac],[examples=false])
AM_CONDITIONAL([INSTALL_EXAMPLES], [test x$examples = xtrue])

AC_ARG_ENABLE([doc],
			  [AS_HELP_STRING([--enable-doc], [Install additional documentation])],
              [case "${enableval}" in
			   yes) doc=true ;;
			   no)  doc=false ;;
			   *) AC_MSG_ERROR([bad value ${enableval} for --enable-doc]) ;;
		   esac],[doc=false])
AM_CONDITIONAL([INSTALL_DOC], [test x$doc = xtrue])

AC_CONFIG_FILES([
				 Makefile
				 doc/Makefile
				 src/Makefile
				 ])

AC_OUTPUT
