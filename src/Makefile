CFLAGS= -O2
CC = gcc
FLAGS=-Wall -g `pkg-config --cflags --libs cairo pangocairo x11 xinerama xrandr imlib2 glib-2.0`
PROGNAME=tint2
FILES=tint.c server.c panel.c config.c taskbar/task.c taskbar/taskbar.c clock/clock.c systray/systraybar.c util/window.c util/area.c

ifndef DESTDIR
	ifndef PREFIX
		BINDIR=/usr/bin
		XDG_CONFIG_DIR=/etc/xdg
		MANDIR=/usr/share/man
	else
		BINDIR=$(PREFIX)/bin
		XDG_CONFIG_DIR=/etc/xdg
		MANDIR=/usr/share/man
	endif
else
	BINDIR=$(DESTDIR)/usr/bin
	XDG_CONFIG_DIR=$(DESTDIR)/etc/xdg
	MANDIR=$(DESTDIR)/usr/share/man
endif


$(PROGNAME): $(FILES) $(SYSTRAYOBJ)
	$(CC) $(CFLAGS) -I. -Iutil -Iclock -Itaskbar -Isystray -o $(PROGNAME) $(FILES) $(FLAGS)
	
install: install-strip

install-strip:
	mkdir -p $(BINDIR)
	mkdir -p $(XDG_CONFIG_DIR)/tint2
	mkdir -p $(MANDIR)
	mkdir -p $(MANDIR)/man1
	strip $(PROGNAME)
	install $(PROGNAME) $(BINDIR)
	cp -f ../tintrc01 $(XDG_CONFIG_DIR)/tint2/tint2rc
	cp -f ../doc/man/tint2.1 $(MANDIR)/man1

install-nostrip:
	mkdir -p $(BINDIR)
	mkdir -p $(XDG_CONFIG_DIR)/tint2
	mkdir -p $(MANDIR)
	mkdir -p $(MANDIR)/man1
	install $(PROGNAME) $(BINDIR)
	cp -f ../tintrc01 $(XDG_CONFIG_DIR)/tint2/tint2rc
	cp -f ../doc/man/tint2.1 $(MANDIR)/man1

uninstall:
	rm -f $(BINDIR)/$(PROGNAME)
	rm -f $(MANDIR)/man1/tint2.1

clean:
	rm -f $(PROGNAME)

.PHONY: clean uninstall install install-strip install-nostrip
