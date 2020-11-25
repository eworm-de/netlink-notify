# netlink-notify - Notify about netlink changes

# commands
CC	:= gcc
MD	:= markdown
CONVERT	:= convert -define png:compression-level=9 -background transparent
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
VERSION := 0.8.0

all: netlink-notify icons README.html

netlink-notify: netlink-notify.c version.h config.h
	$(CC) netlink-notify.c $(CFLAGS) $(LDFLAGS) -o netlink-notify

config.h:
	$(CP) config.def.h config.h

version.h: $(wildcard .git/HEAD .git/index .git/refs/tags/*) Makefile
	printf "#ifndef VERSION\n#define VERSION \"%s\"\n#endif\n" $(shell git describe --long 2>/dev/null || echo ${VERSION}) > $@

icons: icons/netlink-notify-up.png icons/netlink-notify-down.png icons/netlink-notify-address.png icons/netlink-notify-away.png

icons/netlink-notify-up.png: icons/netlink-notify-up.svg
	$(CONVERT) icons/netlink-notify-up.svg icons/netlink-notify-up.png

icons/netlink-notify-down.png: icons/netlink-notify-down.svg
	$(CONVERT) icons/netlink-notify-down.svg icons/netlink-notify-down.png

icons/netlink-notify-address.png: icons/netlink-notify-address.svg
	$(CONVERT) icons/netlink-notify-address.svg icons/netlink-notify-address.png

icons/netlink-notify-away.png: icons/netlink-notify-away.svg
	$(CONVERT) icons/netlink-notify-away.svg icons/netlink-notify-away.png

README.html: README.md
	$(MD) README.md > README.html

install: install-bin install-doc

install-bin: netlink-notify icons
	$(INSTALL) -D -m0755 netlink-notify $(DESTDIR)/usr/bin/netlink-notify
	$(INSTALL) -D -m0644 systemd/netlink-notify.service $(DESTDIR)/usr/lib/systemd/user/netlink-notify.service
	$(INSTALL) -D -m0644 icons/netlink-notify-up.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-up.svg
	$(INSTALL) -D -m0644 icons/netlink-notify-down.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-down.svg
	$(INSTALL) -D -m0644 icons/netlink-notify-address.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-address.svg
	$(INSTALL) -D -m0644 icons/netlink-notify-away.svg $(DESTDIR)/usr/share/icons/hicolor/scalable/status/netlink-notify-away.svg
	$(INSTALL) -D -m0644 icons/netlink-notify-up.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-up.png
	$(INSTALL) -D -m0644 icons/netlink-notify-down.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-down.png
	$(INSTALL) -D -m0644 icons/netlink-notify-address.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-address.png
	$(INSTALL) -D -m0644 icons/netlink-notify-away.png $(DESTDIR)/usr/share/icons/hicolor/48x48/status/netlink-notify-away.png

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
	git archive --format=tar.xz --prefix=netlink-notify-$(VERSION)/ $(VERSION) > netlink-notify-$(VERSION).tar.xz
	gpg --armor --detach-sign --comment netlink-notify-$(VERSION).tar.xz netlink-notify-$(VERSION).tar.xz
	git notes --ref=refs/notes/signatures/tar add -C $$(git archive --format=tar --prefix=netlink-notify-$(VERSION)/ $(VERSION) | gpg --armor --detach-sign --comment netlink-notify-$(VERSION).tar | git hash-object -w --stdin) $(VERSION)
