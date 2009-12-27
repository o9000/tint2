if ENABLE_TINT2CONF
SUBDIRS = tint2conf
endif

AM_CFLAGS += @PANGOCAIRO_CFLAGS@ @PANGO_CFLAGS@ @CAIRO_CFLAGS@ @GLIB2_CFLAGS@ @GOBJECT2_CFLAGS@ @X11_CFLAGS@ @XINERAMA_CFLAGS@ @IMLIB2_CFLAGS@
LIBS = @PANGOCAIRO_LIBS@ @PANGO_LIBS@ @CAIRO_LIBS@ @GLIB2_LIBS@ @GOBJECT2_LIBS@ @X11_LIBS@ @XINERAMA_LIBS@ @IMLIB2_LIBS@ -lrt
INCLUDES = -Iutil -Iclock -Itaskbar -Isystray -Itooltip

bin_PROGRAMS = tint2
tint2_SOURCES = config.c \
				util/area.h \
				util/common.h \
				util/common.c \
				util/area.c \
				util/window.h \
				util/window.c \
				util/timer.h \
				util/timer.c \
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
				taskbar/task.h \
				tooltip/tooltip.c \
				tooltip/tooltip.h

if ENABLE_BATTERY
DEFS += -DENABLE_BATTERY
INCLUDES += -Ibattery
tint2_SOURCES += battery/battery.c \
				 battery/battery.h
endif

