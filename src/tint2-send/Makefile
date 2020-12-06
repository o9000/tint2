ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

tint2-send: tint2-send.c
	$(CC) tint2-send.c -lX11 -o tint2-send

install: tint2-send
	install -m 755 tint2-send $(DESTDIR)/$(PREFIX)/bin/

clean:
	rm -f tint2-send
