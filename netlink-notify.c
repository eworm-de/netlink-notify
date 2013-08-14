/*
 * (C) 2011-2013 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include <asm/types.h>
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
/* we have to undefine this before including net/if.h to
 * notget redefined structs, etc. */
#undef __USE_MISC
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <libnotify/notify.h>

#define PROGNAME	"netlink-notify"

#define MYPROTO NETLINK_ROUTE
#define MYMGRP RTMGRP_IPV4_ROUTE

#define NOTIFICATION_TIMEOUT	10000
#ifndef DEBUG
#define DEBUG	0
#endif

#define ICON_NETWORK_ADDRESS	"netlink-notify-address"
#define ICON_NETWORK_UP		"netlink-notify-up"
#define ICON_NETWORK_DOWN	"netlink-notify-down"
#define ICON_NETWORK_AWAY	"netlink-notify-away"

#define TEXT_TOPIC	"Netlink Notification"
#define TEXT_NEWLINK	"Interface <b>%s</b> is <b>%s</b>."
#define TEXT_NEWADDR	"Interface <b>%s</b> has new %s address\n<b>%s</b>/%d."
#define TEXT_DELLINK	"Interface <b>%s</b> has gone away."

#define CHECK_CONNECTED	IFF_LOWER_UP

struct addresses_seen {
	char *address;
	unsigned char prefix;
	struct addresses_seen *next;
};

struct ifnames {
	char *name;
	unsigned char deleted;
};

char *program;
unsigned int maxinterface = 0;
NotifyNotification ** notifications = NULL;
struct addresses_seen *addresses_seen = NULL;
struct ifnames ** ifnames = NULL;

/*** free_chain ***/
void free_chain(struct addresses_seen *addresses_seen) {
	struct addresses_seen *next;

	/* first struct is allocated by array, do not free */
	if (addresses_seen->address != NULL)
		free(addresses_seen->address);
	addresses_seen->address = NULL;
	addresses_seen->prefix = 0;

	next = addresses_seen->next;
	addresses_seen->next = NULL;
	addresses_seen = next;

	/* free everything else */
	while (addresses_seen != NULL) {
		next = addresses_seen->next;
		if (addresses_seen->address != NULL)
			free(addresses_seen->address);
		free(addresses_seen);
		addresses_seen = next;
	}
}

/*** init_address ***/
void init_address(struct addresses_seen *next) {
	next->address = NULL;
	next->prefix = 0;
	next->next = NULL;
}

/*** add_address ***/
void add_address(struct addresses_seen *next, char *address, unsigned char prefix) {
	while (next->next != NULL) {
		/* just find the last one */
		next = next->next;
	}
	next->address = address;
	next->prefix = prefix;
	next->next = malloc(sizeof(struct addresses_seen));
	init_address(next->next);
}

/*** remove_address ***/
void remove_address(struct addresses_seen *next, char *address, unsigned char prefix) {
	while (next->next != NULL) {
		if (next->address != NULL /* next->address can be NULL if it has been removed from interface */ &&
				strcmp(next->address, address) == 0 && next->prefix == prefix) {
			free(next->address);
			next->address = NULL;
			next->prefix = 0;
		}
		next = next->next;
	}
}

/*** match_address ***/
int match_address(struct addresses_seen *next, char *address, unsigned char prefix) {
	while (next->next != NULL) {
		if (next->address != NULL /* next->address can be NULL if it has been removed from interface */ &&
				strcmp(next->address, address) == 0 && next->prefix == prefix) {
#			if DEBUG
			printf("%s: Matched address: %s/%d\n", program, address, prefix);
#			endif
			return 1;
		}
		next = next->next;
	}

	return 0;
}

/*** newstr_link ***/
char * newstr_link(const char *text, char *interface, unsigned int flags) {
	char *notifystr;

	notifystr = malloc(strlen(text) + strlen(interface) + 4);
	sprintf(notifystr, text, interface, (flags & CHECK_CONNECTED) ? "up" : "down");

	return notifystr;
}

/*** newstr_addr ***/
char * newstr_addr(const char *text, char *interface, unsigned char family, char *ipaddr, unsigned char prefix) {
	char *notifystr;

	notifystr = malloc(strlen(text) + strlen(interface) + strlen(ipaddr));
	sprintf(notifystr, text, interface, family == AF_INET6 ? "IPv6" : "IP", ipaddr, prefix);

	return notifystr;
}

/*** newstr_away ***/
char * newstr_away(const char *text, char *interface) {
	char *notifystr;

	notifystr = malloc(strlen(text) + strlen(interface));
	sprintf(notifystr, text, interface);

	return notifystr;
}

/*** open_netlink ***/
int open_netlink (void) {
	int sock = socket (AF_NETLINK, SOCK_RAW, MYPROTO);
	struct sockaddr_nl addr;

	memset ((void *) &addr, 0, sizeof (addr));

	if (sock < 0)
		return sock;
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid ();
	addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;
	if (bind (sock, (struct sockaddr *) &addr, sizeof (addr)) < 0)
		return -1;
	return sock;
}

/*** read_event ***/
int read_event (int sockint, int (*msg_handler) (struct sockaddr_nl *, struct nlmsghdr *)) {
	int status;
	int ret = 0;
	char buf[4096];
	struct iovec iov = { buf, sizeof buf };
	struct sockaddr_nl snl;
	struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
	struct nlmsghdr *h;

	status = recvmsg (sockint, &msg, 0);

	if (status < 0) {
		/* Socket non-blocking so bail out once we have read everything */
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return ret;

		/* Anything else is an error */
		fprintf (stderr, "read_netlink: Error recvmsg: %d\n", status);
		return status;
	}

	if (status == 0)
		fprintf (stderr, "read_netlink: EOF\n");

	/* We need to handle more than one message per 'recvmsg' */
	for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int) status); h = NLMSG_NEXT (h, status)) {
		/* Finish reading */
		if (h->nlmsg_type == NLMSG_DONE)
			return ret;

		/* Message is some kind of error */
		if (h->nlmsg_type == NLMSG_ERROR) {
			fprintf (stderr, "read_netlink: Message is an error - decode TBD\n");
			return -1;
		}

		/* Call message handler */
		if (msg_handler) {
			ret = (*msg_handler) (&snl, h);
			if (ret < 0) {
				fprintf (stderr, "read_netlink: Message hander error %d\n", ret);
				return ret;
			}
		} else {
			fprintf (stderr, "read_netlink: Error NULL message handler\n");
			return -1;
		}
	}

	return ret;
}

/*** msg_handler ***/
static int msg_handler (struct sockaddr_nl *nl, struct nlmsghdr *msg) {
	char *notifystr = NULL;
	unsigned int errcount = 0;
	GError *error = NULL;
	struct ifaddrmsg *ifa;
	struct ifinfomsg *ifi;
	struct rtattr *rth;
	int rtl;
	char buf[64];
	NotifyNotification *tmp_notification = NULL, *notification = NULL;
	char *icon = NULL;

	ifa = (struct ifaddrmsg *) NLMSG_DATA (msg);
	ifi = (struct ifinfomsg *) NLMSG_DATA (msg);

	/* make sure we have alloced memory for NotifyNotification and addresses_seen struct array */
	if (maxinterface < ifi->ifi_index) {
		notifications = realloc(notifications, (ifi->ifi_index + 1) * sizeof(size_t));
		addresses_seen = realloc(addresses_seen, (ifi->ifi_index + 1) * sizeof(struct addresses_seen));
		ifnames = realloc(ifnames, (ifi->ifi_index + 1) * sizeof(size_t));
		do {
			maxinterface++; /* there is no interface with index 0, so this is safe */

			notifications[maxinterface] =
#				if NOTIFY_CHECK_VERSION(0, 7, 0)
				notify_notification_new(TEXT_TOPIC, NULL, NULL);
#				else
				notify_notification_new(TEXT_TOPIC, NULL, NULL, NULL);
#				endif
			notify_notification_set_category(notifications[maxinterface], PROGNAME);
			notify_notification_set_urgency(notifications[maxinterface], NOTIFY_URGENCY_NORMAL);

			init_address(&addresses_seen[maxinterface]);
			ifnames[maxinterface] = malloc(sizeof(struct ifnames));
			ifnames[maxinterface]->name = NULL;
			ifnames[maxinterface]->deleted = 0;
		} while (maxinterface < ifi->ifi_index);
	} else if (ifnames[ifi->ifi_index]->deleted == 1) {
#		if DEBUG
		printf("%s: Ignoring event for deleted interface %d.\n", program, ifi->ifi_index);
#		endif
		return 0;
	}

	/* make notification point to the array element, will be overwritten 
	 * later when needed for address notification */
	notification = notifications[ifi->ifi_index];

	/* get interface name and store it */
	if (ifnames[ifi->ifi_index]->name == NULL) {
		ifnames[ifi->ifi_index]->name = realloc(ifnames[ifi->ifi_index]->name, IFNAMSIZ * sizeof(char));
		sprintf(ifnames[ifi->ifi_index]->name, "(unknown)");
	}
	if_indextoname(ifi->ifi_index, ifnames[ifi->ifi_index]->name);

#	if DEBUG
	printf("%s: Interface %s (%d), flags: %x, msg type: %d\n",
			program, ifnames[ifi->ifi_index]->name, ifi->ifi_index, ifa->ifa_flags, msg->nlmsg_type);
#	endif

	switch (msg->nlmsg_type) {
		/* just return for cases we want to ignore
		 * use break if a notification has to be displayed */
		case RTM_NEWADDR:
			rth = IFA_RTA (ifa);
			rtl = IFA_PAYLOAD (msg);

			while (rtl && RTA_OK (rth, rtl)) {
				if ((rth->rta_type == IFA_LOCAL /* IPv4 */
						|| rth->rta_type == IFA_ADDRESS /* IPv6 */)
						&& ifa->ifa_scope == RT_SCOPE_UNIVERSE /* no IPv6 scope link */) {
					inet_ntop(ifa->ifa_family, RTA_DATA (rth), buf, sizeof(buf));

					/* check if we already notified about this address */
					if (match_address(&addresses_seen[ifi->ifi_index], buf, ifa->ifa_prefixlen))
						break;

					/* add address to struct */
					add_address(&addresses_seen[ifi->ifi_index], strdup(buf), ifa->ifa_prefixlen);

					/* display notification */
					notifystr = newstr_addr(TEXT_NEWADDR, ifnames[ifi->ifi_index]->name,
						ifa->ifa_family, buf, ifa->ifa_prefixlen);

					/* we are done, no need to run more loops */
					break;
				}
				rth = RTA_NEXT (rth, rtl);
			}
			if (notifystr == NULL) {
				return 0;
			}

			/* do we want new notification, not update the notification about link status */
			tmp_notification =
#				if NOTIFY_CHECK_VERSION(0, 7, 0)
				notify_notification_new(TEXT_TOPIC, NULL, NULL);
#				else
				notify_notification_new(TEXT_TOPIC, NULL, NULL, NULL);
#				endif
			notify_notification_set_category(tmp_notification, PROGNAME);
			notify_notification_set_urgency(tmp_notification, NOTIFY_URGENCY_NORMAL);

			notification = tmp_notification;

			icon = ICON_NETWORK_ADDRESS;

			break;
		case RTM_DELADDR:
			rth = IFA_RTA (ifa);
			rtl = IFA_PAYLOAD (msg);

			while (rtl && RTA_OK (rth, rtl)) {
				if ((rth->rta_type == IFA_LOCAL /* IPv4 */
						|| rth->rta_type == IFA_ADDRESS /* IPv6 */)
						&& ifa->ifa_scope == RT_SCOPE_UNIVERSE /* no IPv6 scope link */) {
					inet_ntop(ifa->ifa_family, RTA_DATA (rth), buf, sizeof(buf));
					remove_address(&addresses_seen[ifi->ifi_index], buf, ifa->ifa_prefixlen);

					/* we are done, no need to run more loops */
					break;
				}
				rth = RTA_NEXT (rth, rtl);
			}

			return 0;
		case RTM_NEWROUTE:
			return 0;
		case RTM_DELROUTE:
			return 0;
		case RTM_NEWLINK:
			notifystr = newstr_link(TEXT_NEWLINK, ifnames[ifi->ifi_index]->name, ifi->ifi_flags);

			icon = ifi->ifi_flags & CHECK_CONNECTED ? ICON_NETWORK_UP : ICON_NETWORK_DOWN;

			/* free only if interface goes down */
			if (!(ifi->ifi_flags & CHECK_CONNECTED))
				free_chain(&addresses_seen[ifi->ifi_index]);

			break;
		case RTM_DELLINK:
			notifystr = newstr_away(TEXT_DELLINK, ifnames[ifi->ifi_index]->name);

			icon = ICON_NETWORK_AWAY;

			free_chain(&addresses_seen[ifi->ifi_index]);
			free(ifnames[ifi->ifi_index]->name);
			/* marking interface deleted makes events for this interface to be ignored */
			ifnames[ifi->ifi_index]->deleted = 1;
			tmp_notification = notification;

			break;
		default:
			/* we should not get here... */
			fprintf(stderr, "msg_handler: Unknown netlink nlmsg_type %d\n", msg->nlmsg_type);
			return 0;
	}

#	if DEBUG
	printf("%s: %s\n", program, notifystr);
#	endif

	notify_notification_update(notification, TEXT_TOPIC, notifystr, icon);

	while (!notify_notification_show (notification, &error)) {
		if (errcount > 1) {
			fprintf(stderr, "%s: Looks like we can not reconnect to notification daemon... Exiting.\n", program);
			exit(EXIT_FAILURE);
		} else {
			g_printerr("%s: Error \"%s\" while trying to show notification. Trying to reconnect.\n", program, error->message);
			errcount++;

			g_error_free (error);
			error = NULL;

			notify_uninit ();

			usleep (500 * 1000);

			if (!notify_init (PROGNAME)) {
				fprintf(stderr, "%s: Can't create notify.\n", program);
				exit(EXIT_FAILURE);
			}
		}
	}

	if (tmp_notification)
		g_object_unref(G_OBJECT(tmp_notification));
	errcount = 0;
	free(notifystr);

	return 0;
}

/*** main ***/
int main (int argc, char **argv) {
	int nls;

	program = argv[0];
	printf ("%s: %s v%s (compiled: " __DATE__ ", " __TIME__
#			if DEBUG
			", with debug output"
#			endif
			")\n", argv[0], PROGNAME, VERSION);

	nls = open_netlink ();
	if (nls < 0) {
		fprintf (stderr, "%s: Error opening netlin socket!", argv[0]);
		exit (EXIT_FAILURE);
	}

	if (!notify_init (PROGNAME)) {
		fprintf (stderr, "%s: Can't create notify.\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	while (1)
		read_event (nls, msg_handler);

	return EXIT_SUCCESS;
}
