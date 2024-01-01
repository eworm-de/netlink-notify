/*
 * (C) 2011-2024 by Christian Hesse <mail@eworm.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#ifndef NETLINK_NOTIFY_H
#define NETLINK_NOTIFY_H

#define _GNU_SOURCE

#include <asm/types.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/wireless.h>
/* we have to undefine this before including net/if.h to
 * notget redefined structs, etc. */
#undef __USE_MISC
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

/* systemd headers */
#ifdef HAVE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include <libnotify/notify.h>

#include "version.h"
#include "config.h"

#define PROGNAME	"netlink-notify"

#define CHECK_CONNECTED	IFF_LOWER_UP

struct addresses_seen {
	char address[INET6_ADDRSTRLEN];
	unsigned char prefix;
	struct addresses_seen *next;
};

struct ifs {
	char name[IF_NAMESIZE];
	int state;
	uint8_t deleted;
	struct addresses_seen *addresses_seen;
	NotifyNotification *notification;
};

/*** free_addresses ***/
void free_addresses(struct addresses_seen *addresses_seen);

/*** add_address ***/
struct addresses_seen * add_address(struct addresses_seen *addresses_seen, const char *address, const unsigned char prefix);

/*** remove_address ***/
struct addresses_seen * remove_address(struct addresses_seen *addresses_seen, const char *address, const unsigned char prefix);

/*** match_address ***/
int match_address(struct addresses_seen *addresses_seen, const char *address, const unsigned char prefix);

/*** list_addresses ***/
void list_addresses(struct addresses_seen *addresses_seen, const char *interface);

/*** get_ssid ***/
void get_ssid(const char *interface, char *essid);

/*** newstr_link ***/
char * newstr_link(const char *interface, const unsigned int flags);

/*** newstr_addr ***/
char * newstr_addr(const char *interface, const unsigned char family, const char *ipaddr, const unsigned char prefix);

/*** newstr_away ***/
char * newstr_away(const char *interface);

/*** open_netlink ***/
int open_netlink (void);

/*** read_event ***/
int read_event (int sockint);

/*** msg_handler ***/
int msg_handler (struct sockaddr_nl *nl, struct nlmsghdr *msg);

/*** received_signal ***/
void received_signal(int signal);

/*** main ***/
int main (int argc, char **argv);

#endif /* NETLINK_NOTIFY_H */
