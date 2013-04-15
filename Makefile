# netlink-notify - Notify about netlink changes

CC	:= gcc
CFLAGS	+= $(shell pkg-config --cflags --libs libmnl) $(shell pkg-config --cflags --libs libnotify)
VERSION	= $(shell git describe --tags --long)

all: netlink-notify.c
	$(CC) $(CFLAGS) -o netlink-notify netlink-notify.c \
		-DVERSION="\"$(VERSION)\""

clean:
	/bin/rm -f *.o *~ netlink-notify
