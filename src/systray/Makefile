#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
#                                                                       #
# Change these values to customize your installation and build process  #
#                                                                       #
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

# Change this PREFIX to where you want docker to be installed
PREFIX=/usr/local
# Change this XLIBPATH to point to your X11 development package's installation
XLIBPATH=/usr/X11R6/lib

# Sets some flags for stricter compiling
CFLAGS=-pedantic -Wall -W -O

#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#
#                                                                 #
#  Leave the rest of the Makefile alone if you want it to build!  #
#                                                                 #
#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#-#

PACKAGE=docker
VERSION=1.5

target=docker
sources=docker.c kde.c icons.c xproperty.c net.c
headers=docker.h kde.h icons.h xproperty.h net.h version.h
extra=README COPYING version.h.in

all: $(target) $(sources) $(headers)
	@echo Build Successful

$(target): $(sources:.c=.o)
	$(CC) $(CFLAGS) -L$(XLIBPATH) -lX11 \
		`pkg-config --libs glib-2.0` $^ -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) `pkg-config --cflags glib-2.0` $<

version.h: version.h.in Makefile
	sed -e "s/@VERSION@/$(VERSION)/" version.h.in > $@

install: all
	install $(target) $(PREFIX)/bin/$(target)

uninstall:
	rm -f $(PREFIX)/$(target)

clean:
	rm -rf .dist
	rm -f core *.o .\#* *\~ $(target)

distclean: clean
	rm -f version.h
	rm -f $(PACKAGE)-*.tar.gz

dist: Makefile $(sources) $(headers) $(extra)
	mkdir -p .dist/$(PACKAGE)-$(VERSION) && \
	cp $^ .dist/$(PACKAGE)-$(VERSION) && \
	tar -c -z -C .dist -f \
		$(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-$(VERSION) && \
	rm -rf .dist

love: $(sources)
	touch $^

# local dependancies
docker.o: docker.c version.h kde.h icons.h docker.h net.h
icons.o: icons.c icons.h docker.h
kde.o: kde.c kde.h docker.h xproperty.h
net.o: net.c net.h docker.h icons.h
xproperty.o: xproperty.c xproperty.h docker.h
