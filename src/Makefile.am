AM_CFLAGS = -Wall
AM_CFLAGS += @PANGOCAIRO_CFLAGS@ @PANGO_CFLAGS@ @CAIRO_CFLAGS@ @GLIB2_CFLAGS@ @GOBJECT2_CFLAGS@ @X11_CFLAGS@ @XINERAMA_CFLAGS@ @IMLIB2_CFLAGS@

AM_LDFLAGS = -Wl,--as-needed

LIBS = @PANGOCAIRO_LIBS@ @PANGO_LIBS@ @CAIRO_LIBS@ @GLIB2_LIBS@ @GOBJECT2_LIBS@ @X11_LIBS@ @XINERAMA_LIBS@ @IMLIB2_LIBS@
INCLUDES = -Iutil -Iclock -Itaskbar -Isystray

bin_PROGRAMS = tint2
tint2_SOURCES = config.c \
				util/area.h \
				util/common.h \
				util/area.c \
				util/window.h \
				util/window.c \
				panel.h \
				clock/clock.c \
				clock/clock.h \
				config.h \
				tint.c \
				panel.c \
				server.c \
				server.h \
				systray/systraybar.h \
				systray/systraybar.c \
				taskbar/taskbar.c \
				taskbar/task.c \
				taskbar/taskbar.h \
				taskbar/task.h 

if ENABLE_BATTERY
AM_CFLAGS += -DENABLE_BATTERY
INCLUDES += -Ibattery
tint2_SOURCES += battery/battery.c \
				 battery/battery.h
endif

DISTCLEANFILES = Makefile.in


