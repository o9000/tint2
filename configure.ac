#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_INIT([tint2], [0.7], [http://code.google.com/p/tint2/issues])

LT_INIT
AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/tint.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_LANG([C])
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

# Ensure that Imlib2 has been built with X support and reset LIBS since AC_CHECK_LIB adds -lImlib2 on success.
LIBS_SAVED=$LIBS
AC_CHECK_LIB([Imlib2], [imlib_context_set_display], [],
			 AC_MSG_ERROR([Imlib2 must be built with X support]))
LIBS=$LIBS_SAVED

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

AC_ARG_ENABLE([battery],
			  [AS_HELP_STRING([--disable-battery], [Disable battery status plugin])],
			  [case "${enableval}" in
			   yes) battery=true ;;
			   no) battery=false ;;
			   *) AC_MSG_ERROR([bad value ${enableval} for --disable-battery]) ;;
		   esac], [battery=true])
AM_CONDITIONAL([ENABLE_BATTERY], [test x$battery = xtrue])

AC_ARG_ENABLE([examples],
			  [AS_HELP_STRING([--enable-examples], [Install additional tin2rc examples])],
			  [case "${enableval}" in
			   yes) examples=true ;;
			   no)  examples=false ;;
			   *) AC_MSG_ERROR([bad value ${enableval} for --enable-examples]) ;;
		   esac],[examples=false])
AM_CONDITIONAL([INSTALL_EXAMPLES], [test x$examples = xtrue])

AC_CONFIG_FILES([
				 Makefile
				 doc/Makefile
				 src/Makefile
				 ])

AC_OUTPUT
