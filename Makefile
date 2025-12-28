# netlink-notify - Notify about netlink changes

# commands
CC	:= gcc
MD	:= markdown
RESVG	:= resvg
OXIPNG	:= oxipng
INSTALL	:= install
CP	:= cp
RM	:= rm

# flags
CFLAGS	+= -std=c11 -O2 -fPIC -Wall -Werror
CFLAGS	+= $(shell pkg-config --cflags --libs libnotify)
CFLAGS_SYSTEMD := $(shell pkg-config --cflags --libs libsystemd 2>/dev/null)
ifneq ($(CFLAGS_SYSTEMD),)
CFLAGS	+= -DHAVE_SYSTEMD $(CFLAGS_SYSTEMD)
endif
LDFLAGS	+= -Wl,-z,now -Wl,-z,relro -pie

# this is just a fallback in case you do not use git but downloaded
# a release tarball...
DISTVER := 0.8.2
VERSION ?= $(shell git describe --long 2>/dev/null || echo $(DISTVER))

all: netlink-notify icons README.html

netlink-notify: netlink-notify.c version.h config.h
	$(CC) netlink-notify.c $(CFLAGS) $(LDFLAGS) -o netlink-notify

config.h:
	$(CP) config.def.h config.h

version.h: $(wildcard .git/HEAD .git/index .git/refs/tags/*) Makefile
	printf "#ifndef VERSION\n#define VERSION \"%s\"\n#endif\n" "$(VERSION)" > $@

icons: icons/netlink-notify-up.png icons/netlink-notify-down.png icons/netlink-notify-address.png icons/netlink-notify-away.png

%.png: %.svg
	$(RESVG) $< -c | $(OXIPNG) - > $@

README.html: README.md
	$(MD) README.md > README.html

install: install-bin install-doc install-units

install-bin: netlink-notify icons
	$(INSTALL) -D -m0755 netlink-notify $(DESTDIR)/usr/bin/netlink-notify
	$(INSTALL) -D -m0644 icons/netlink-notify-up.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-up.svg
	$(INSTALL) -D -m0644 icons/netlink-notify-down.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-down.svg
	$(INSTALL) -D -m0644 icons/netlink-notify-address.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-address.svg
	$(INSTALL) -D -m0644 icons/netlink-notify-away.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-away.svg
	$(INSTALL) -D -m0644 icons/netlink-notify-up.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-up.png
	$(INSTALL) -D -m0644 icons/netlink-notify-down.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-down.png
	$(INSTALL) -D -m0644 icons/netlink-notify-address.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-address.png
	$(INSTALL) -D -m0644 icons/netlink-notify-away.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-away.png

install-units:
ifneq ($(CFLAGS_SYSTEMD),)
	$(INSTALL) -D -m0644 systemd/netlink-notify.service $(DESTDIR)/usr/lib/systemd/user/netlink-notify.service
endif

install-doc: README.html
	$(INSTALL) -D -m0644 README.md $(DESTDIR)/usr/share/doc/netlink-notify/README.md
	$(INSTALL) -D -m0644 README.html $(DESTDIR)/usr/share/doc/netlink-notify/README.html
	$(INSTALL) -D -m0644 screenshots/away.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshots/away.png
	$(INSTALL) -D -m0644 screenshots/down.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshots/down.png
	$(INSTALL) -D -m0644 screenshots/ip.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshots/ip.png
	$(INSTALL) -D -m0644 screenshots/ipv6.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshots/ipv6.png
	$(INSTALL) -D -m0644 screenshots/up.png $(DESTDIR)/usr/share/doc/netlink-notify/screenshots/up.png

clean:
	$(RM) -f *.o *~ README.html netlink-notify version.h

distclean:
	$(RM) -f *.o *~ README.html netlink-notify version.h config.h

release:
	git archive --format=tar.xz --prefix=netlink-notify-$(DISTVER)/ $(DISTVER) > netlink-notify-$(DISTVER).tar.xz
	gpg --armor --detach-sign --comment netlink-notify-$(DISTVER).tar.xz netlink-notify-$(DISTVER).tar.xz
	git notes --ref=refs/notes/signatures/tar add -C $$(git archive --format=tar --prefix=netlink-notify-$(DISTVER)/ $(DISTVER) | gpg --armor --detach-sign --comment netlink-notify-$(DISTVER).tar | git hash-object -w --stdin) $(DISTVER)
