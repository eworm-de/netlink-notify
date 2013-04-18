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

#include <ifaddrs.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
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

#define ICON_NETWORK_CONNECTED		"netlink-notify-connected"
#define ICON_NETWORK_DISCONNECTED	"netlink-notify-disconnected"

#define TEXT_TOPIC	"Netlink Notification"
#define TEXT_NEWLINK	"Interface <b>%s</b> is <b>%s</b>."
#define TEXT_NEWADDR	"Interface <b>%s</b> is <b>%s</b>,\nnew address <b>%s</b>/%d."
#define TEXT_DELLINK	"An interface has gone away."

#define TEXT_NONE	"(NONE)"

#define CHECK_CONNECTED	IFF_LOWER_UP

char *program;
unsigned int maxinterface = 0;
NotifyNotification ** notification;
     
/*** newstr_link ***/
char * newstr_link(char *text, char *interface, unsigned int flags) {
	char *notifystr;

	notifystr = malloc(strlen(text) + strlen(interface) + 4);
	sprintf(notifystr, text, interface, (flags & CHECK_CONNECTED) ? "up" : "down");

	return notifystr;
}

/*** newstr_addr ***/
char * newstr_addr(char *text, char *interface, unsigned int flags, unsigned char family, void *ipaddr, unsigned char prefix) {
	char *notifystr;
	char buf[64];

	inet_ntop(family, ipaddr, buf, sizeof(buf));
	notifystr = malloc(strlen(text) + strlen(interface) + strlen(buf));
	sprintf(notifystr, text, interface, (flags & CHECK_CONNECTED) ? "up" : "down", buf, prefix);

	return notifystr;
}

/*** newstr_away ***/
char * newstr_away(char *text) {
	char *notifystr;

	notifystr = malloc(strlen(text));
	sprintf(notifystr, text);

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
	char name[IFNAMSIZ];

	ifa = (struct ifaddrmsg *) NLMSG_DATA (msg);
	ifi = (struct ifinfomsg *) NLMSG_DATA (msg);

	if_indextoname(ifi->ifi_index, name);

	/* make sure we have alloced memory for the struct array */
	if (maxinterface < ifi->ifi_index) {
		notification = realloc(notification, (ifi->ifi_index + 1) * sizeof(NotifyNotification));
		for(; maxinterface < ifi->ifi_index; maxinterface++)
			notification[maxinterface] = NULL;
	}

	switch (msg->nlmsg_type) {
		/* just return for cases we want to ignore */
		case RTM_NEWADDR:
			rth = IFA_RTA (ifa);
			rtl = IFA_PAYLOAD (msg);
		
			while (rtl && RTA_OK (rth, rtl)) {
				if (rth->rta_type == IFA_LOCAL)
					notifystr = newstr_addr(TEXT_NEWADDR, name, ifi->ifi_flags,
						ifa->ifa_family, RTA_DATA (rth), ifa->ifa_prefixlen);
				rth = RTA_NEXT (rth, rtl);
			}
			if (notifystr == NULL) {
				return 0;
			}
#if DEUBG
			puts (notifystr);
#endif
			break;
		case RTM_DELADDR:
			return 0;
		case RTM_NEWROUTE:
			return 0;
		case RTM_DELROUTE:
			return 0;
		case RTM_NEWLINK:
			notifystr = newstr_link(TEXT_NEWLINK, name, ifi->ifi_flags);
#if DEBUG
			puts (notifystr);
#endif
			break;
		case RTM_DELLINK:
			notifystr = newstr_away(TEXT_DELLINK);
#if DEBUG
			puts (notifystr);
#endif
			break;
		default:
			/* we should not get here... */
			fprintf(stderr, "msg_handler: Unknown netlink nlmsg_type %d\n", msg->nlmsg_type);
			return 0;
	}

	if (notification[ifi->ifi_index] == NULL)
		notification[ifi->ifi_index] = notify_notification_new(TEXT_TOPIC, notifystr,
			(ifi->ifi_flags & CHECK_CONNECTED ? ICON_NETWORK_CONNECTED : ICON_NETWORK_DISCONNECTED));
	else
		notify_notification_update(notification[ifi->ifi_index], TEXT_TOPIC, notifystr,
			(ifi->ifi_flags & CHECK_CONNECTED ? ICON_NETWORK_CONNECTED : ICON_NETWORK_DISCONNECTED));

	notify_notification_set_timeout(notification[ifi->ifi_index], NOTIFICATION_TIMEOUT);
	notify_notification_set_category(notification[ifi->ifi_index], PROGNAME);
	notify_notification_set_urgency(notification[ifi->ifi_index], NOTIFY_URGENCY_NORMAL);

	while (!notify_notification_show (notification[ifi->ifi_index], &error)) {
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
	errcount = 0;
	free(notifystr);

	return 0;
}

/*** main ***/
int main (int argc, char **argv) {
	int nls;

	program = argv[0];
	printf ("%s: %s v%s (compiled: " __DATE__ ", " __TIME__ ")\n", argv[0], PROGNAME, VERSION);

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
