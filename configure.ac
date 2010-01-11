#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([tint2], [0.8], [http://code.google.com/p/tint2/issues])

AM_INIT_AUTOMAKE

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/tint.c])
AC_CONFIG_HEADERS([config.h])

# tint2
AC_ARG_ENABLE([battery],
	[AS_HELP_STRING([--disable-battery], [Disable battery status plugin])],
		[case "${enableval}" in
			yes) battery=true ;;
			no) battery=false ;;
			*) AC_MSG_ERROR([bad value ${enableval} for --disable-battery]) ;;
		esac], [battery=true])
AM_CONDITIONAL([ENABLE_BATTERY], [test x$battery = xtrue])
#

AC_ARG_ENABLE([examples],
	[AS_HELP_STRING([--enable-examples], [Install additional tin2rc examples])],
		[case "${enableval}" in
			yes) examples=true ;;
			no)  examples=false ;;
			*) AC_MSG_ERROR([bad value ${enableval} for --enable-examples]) ;;
		esac],[examples=false])
AM_CONDITIONAL([INSTALL_EXAMPLES], [test x$examples = xtrue])

# tint2conf
AC_ARG_ENABLE([tint2conf],
	[AS_HELP_STRING([--enable-tint2conf], [Enable tint2conf build, a GTK+2 theme switcher for tint2])],
		[case "${enableval}" in
			yes) tint2conf=true ;;
			no)  tint2conf=false ;;
			*) AC_MSG_ERROR([bad value ${enableval} for --enable-tint2conf]) ;;
		esac],[tint2conf=false])
AM_CONDITIONAL([ENABLE_TINT2CONF], [test x$tint2conf = xtrue])
#

# Checks for programs.
AC_LANG([C])
AC_PROG_CC
PKG_PROG_PKG_CONFIG

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

PKG_CHECK_MODULES([X11], [x11 xdamage xcomposite])
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
AC_CHECK_HEADERS([fcntl.h locale.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([alarm gettimeofday memset select setlocale strcasecmp strchr strdup])

# tint2conf (experimental)
if test "x$tint2conf" = xtrue;
then
	# We want just link against stuff we really need so thats why I don't use PKG_CHECK_MODULES.
	# gtk/glib is a bit horrible because we have to add -I directives for various features (like cairo, pango etc.)
	# even if we don't need them.
	# Sorry if that might be confusing %-)

	AC_CHECK_FUNCS([strndup strrchr strstr])
	# Save LIBS value and clear the variable, AC_CHECK_LIB will append all libs to LIBS on success.
	LIBS_SAVED=$LIBS
	LIBS=

	AC_CHECK_LIB([pthread], [pthread_create], [],
				 [AC_MSG_ERROR([libpthread is missing, usually provided by glibc])])
	AC_CHECK_LIB([glib-2.0], [g_free], [],
				 [AC_MSG_ERROR([glib-2.x is missing])])
	AC_CHECK_LIB([gobject-2.0], [g_signal_connect_data], [],
				 [AC_MSG_ERROR([libgobject-2.0 is missing, usually provided by glib-2.x])])
	AC_CHECK_LIB([gtk-x11-2.0], [gtk_main], [],
				 [AC_MSG_ERROR([gtk+-2.x is missing or not built with X support])])
	AC_CHECK_LIB([gthread-2.0], [g_thread_init], [],
				 [AC_MSG_ERROR([libgthread-2.0 is missing, usually provided by glib-2.x])])

	TINT2CONF_LIBS="${LIBS} ${PTHREAD_LIB} ${GLIB2_LIB} ${GOBJECT2_LIB} ${IMLIB2_LIBS}"
	LIBS=$LIBS_SAVED

	TINT2CONF_CFLAGS="$(${PKG_CONFIG} --cflags gtk+-x11-2.0 glib-2.0 gobject-2.0 gthread-2.0) ${IMLIB2_CFLAGS}"
	AC_SUBST(TINT2CONF_CFLAGS)
	AC_SUBST(TINT2CONF_LIBS)

	# Python stuff
	AC_PATH_PROG([__PYTHON], [python])
	if test -z "${__PYTHON}";
	then
		AC_MSG_ERROR([tint2conf requires python to be installed])
	fi

	PKG_CHECK_EXISTS([pygtk-2.0],,[AC_MSG_ERROR([tint2conf requires >=pygtk-2.0])])
fi
#

#
AM_CFLAGS="-Wall"
AM_LDFLAGS="-Wl,--as-needed"
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_LDFLAGS)

DISTCLEANFILES="Makefile.in"
AC_SUBST(DISTCLEANFILES)

AC_CONFIG_FILES([Makefile
				 doc/Makefile
				 sample/Makefile
				 src/Makefile
				 src/tint2conf/Makefile])

AC_OUTPUT
