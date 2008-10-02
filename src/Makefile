#CFLAGS="-O2"
FLAGS=-Wall -g `pkg-config --cflags --libs cairo pangocairo x11 xinerama imlib2 glib-2.0`
PROGNAME=tint2
FILES=tint.c server.c panel.c config.c taskbar/task.c taskbar/taskbar.c clock/clock.c systray/docker.c systray/icons.c systray/kde.c systray/net.c systray/xproperty.c util/window.c util/area.c

ifndef DESTDIR
	ifndef PREFIX
		BINDIR=/usr/bin
		XDG_CONFIG_DIR=/etc/xdg
	else
		BINDIR=$(PREFIX)/bin
		XDG_CONFIG_DIR=/etc/xdg
	endif
else
	BINDIR=$(DESTDIR)/usr/bin
	XDG_CONFIG_DIR=$(DESTDIR)/etc/xdg
endif


$(PROGNAME): $(FILES) $(SYSTRAYOBJ)
	$(CC) $(CFLAGS) $(FLAGS) -I. -Iutil -Iclock -Itaskbar -Isystray -o $(PROGNAME) $(FILES) 
	strip $(PROGNAME)

install:
	mkdir -p $(BINDIR)
	mkdir -p $(XDG_CONFIG_DIR)/tint2
	install $(PROGNAME) $(BINDIR)
	cp -f ../tintrc06 $(XDG_CONFIG_DIR)/tint2/tint2rc
	cp -f ../doc/man/tint2.1 /usr/man/man1

uninstall:
	rm -f $(BINDIR)/$(PROGNAME)

clean:
	rm -f $(PROGNAME)

