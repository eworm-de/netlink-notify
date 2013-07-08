# netlink-notify - Notify about netlink changes

CC	:= gcc
CONVERT	:= inkscape --export-png
INSTALL	:= install
CFLAGS	+= -O2 -Wall -Werror
CFLAGS	+= $(shell pkg-config --cflags --libs libnotify)
VERSION	= $(shell git describe --tags --long)

all: binary icons

binary: netlink-notify.c
	$(CC) $(CFLAGS) -o netlink-notify netlink-notify.c \
		-DVERSION="\"$(VERSION)\""

icons:
	$(CONVERT) netlink-notify-up.png netlink-notify-up.svg
	$(CONVERT) netlink-notify-down.png netlink-notify-down.svg
	$(CONVERT) netlink-notify-address.png netlink-notify-address.svg

install:
	$(INSTALL) -D -m0755 netlink-notify $(DESTDIR)/usr/bin/netlink-notify
	$(INSTALL) -D -m0644 netlink-notify.desktop $(DESTDIR)/etc/xdg/autostart/netlink-notify.desktop
	$(INSTALL) -D -m0755 netlink-notify-up.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-up.svg
	$(INSTALL) -D -m0755 netlink-notify-down.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-down.svg
	$(INSTALL) -D -m0755 netlink-notify-address.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-address.svg
	$(INSTALL) -D -m0755 netlink-notify-up.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-up.png
	$(INSTALL) -D -m0755 netlink-notify-down.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-down.png
	$(INSTALL) -D -m0755 netlink-notify-address.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-address.png

clean:
	/bin/rm -f *.o *.png *~ netlink-notify
