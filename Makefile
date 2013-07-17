# netlink-notify - Notify about netlink changes

CC	:= gcc
MD	:= markdown
CONVERT	:= inkscape --export-png
INSTALL	:= install
RM	:= rm
CFLAGS	+= -O2 -Wall -Werror
CFLAGS	+= $(shell pkg-config --cflags --libs libnotify)
VERSION	= $(shell git describe --tags --long)

all: netlink-notify icons README.html

netlink-notify: netlink-notify.c
	$(CC) $(CFLAGS) -o netlink-notify netlink-notify.c \
		-DVERSION="\"$(VERSION)\""

icons:
	$(CONVERT) netlink-notify-up.png netlink-notify-up.svg
	$(CONVERT) netlink-notify-down.png netlink-notify-down.svg
	$(CONVERT) netlink-notify-address.png netlink-notify-address.svg
	$(CONVERT) netlink-notify-away.png netlink-notify-away.svg

README.html: README.md
	$(MD) README.md > README.html

install:
	$(INSTALL) -D -m0755 netlink-notify $(DESTDIR)/usr/bin/netlink-notify
	$(INSTALL) -D -m0644 netlink-notify.desktop $(DESTDIR)/etc/xdg/autostart/netlink-notify.desktop
	$(INSTALL) -D -m0755 netlink-notify-up.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-up.svg
	$(INSTALL) -D -m0755 netlink-notify-down.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-down.svg
	$(INSTALL) -D -m0755 netlink-notify-address.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-address.svg
	$(INSTALL) -D -m0755 netlink-notify-away.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-away.svg
	$(INSTALL) -D -m0755 netlink-notify-up.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-up.png
	$(INSTALL) -D -m0755 netlink-notify-down.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-down.png
	$(INSTALL) -D -m0755 netlink-notify-address.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-address.png
	$(INSTALL) -D -m0755 netlink-notify-away.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-away.png
	$(INSTALL) -D -m0644 README.md $(DESTDIR)/usr/share/doc/netlink-notify/README.md
	$(INSTALL) -D -m0644 README.html $(DESTDIR)/usr/share/doc/netlink-notify/README.html
	$(INSTALL) -D -m0644 screenshot-away.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshot-away.png
	$(INSTALL) -D -m0644 screenshot-down.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshot-down.png
	$(INSTALL) -D -m0644 screenshot-ip.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshot-ip.png
	$(INSTALL) -D -m0644 screenshot-ipv6.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshot-ipv6.png
	$(INSTALL) -D -m0644 screenshot-up.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshot-up.png

clean:
	$(RM) -f *.o *~ netlink-notify-*.png README.html netlink-notify
