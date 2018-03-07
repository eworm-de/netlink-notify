/*
 * (C) 2011-2018 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#include "netlink-notify.h"

const static char optstring[] = "ht:vV";
const static struct option options_long[] = {
	/* name		has_arg			flag	val */
	{ "help",	no_argument,		NULL,	'h' },
	{ "timeout",	required_argument,	NULL,	't' },
	{ "verbose",	no_argument,		NULL,	'v' },
	{ "version",	no_argument,		NULL,	'V' },
	{ 0, 0, 0, 0 }
};

char *program;
unsigned int maxinterface = 0;
struct ifs * ifs = NULL;
uint8_t verbose = 0;
uint8_t doexit = 0;
unsigned int notification_timeout = NOTIFICATION_TIMEOUT;

/*** free_addresses ***/
void free_addresses(struct addresses_seen *addresses_seen) {
	struct addresses_seen *next = NULL;

	/* free everything else */
	while (addresses_seen != NULL) {
		next = addresses_seen->next;
		free(addresses_seen);
		addresses_seen = next;
	}
}

/*** add_address ***/
struct addresses_seen * add_address(struct addresses_seen *addresses_seen, const char *address, unsigned char prefix) {
	struct addresses_seen *first = addresses_seen;

	if (addresses_seen == NULL) {
		addresses_seen = malloc(sizeof(struct addresses_seen));
		strcpy(addresses_seen->address, address);
		addresses_seen->prefix = prefix;
		addresses_seen->next = NULL;
		return addresses_seen;
	}

	while (addresses_seen->next != NULL) {
		/* just find the last one */
		addresses_seen = addresses_seen->next;
	}

	addresses_seen->next = malloc(sizeof(struct addresses_seen));
	addresses_seen = addresses_seen->next;
	strcpy(addresses_seen->address, address);
	addresses_seen->prefix = prefix;
	addresses_seen->next = NULL;

	return first;
}

/*** remove_address ***/
struct addresses_seen * remove_address(struct addresses_seen *addresses_seen, const char *address, unsigned char prefix) {
	struct addresses_seen *first = addresses_seen, *last = NULL;

	/* no addresses, just return NULL */
	if (addresses_seen == NULL)
		return NULL;

	/* first address matches, return new start */
	if (strcmp(addresses_seen->address, address) == 0 && addresses_seen->prefix == prefix) {
		first = addresses_seen->next;
		free(addresses_seen);
		return first;
	}

	/* find the address and remove it */
	while (addresses_seen != NULL) {
		if (strcmp(addresses_seen->address, address) == 0 && addresses_seen->prefix == prefix) {
			last->next = addresses_seen->next;
			free(addresses_seen);
			break;
		}
		last = addresses_seen;
		addresses_seen = addresses_seen->next;
	}
	return first;
}

/*** match_address ***/
int match_address(struct addresses_seen *addresses_seen, const char *address, unsigned char prefix) {
	while (addresses_seen != NULL) {
		if (strcmp(addresses_seen->address, address) == 0 && addresses_seen->prefix == prefix) {
			return 1;
		}
		addresses_seen = addresses_seen->next;
	}
	return 0;
}

/*** list_addresses ***/
void list_addresses(struct addresses_seen *addresses_seen, char *interface) {
	printf("%s: Addresses seen for interface %s:", program, interface);
	while (addresses_seen != NULL) {
		printf(" %s/%d", addresses_seen->address, addresses_seen->prefix);
		addresses_seen = addresses_seen->next;
	}
	putchar('\n');
}

/*** get_ssid ***/
void get_ssid(const char *interface, char *essid) {
	int sockfd;
	struct iwreq wreq;

	memset(&wreq, 0, sizeof(struct iwreq));
	snprintf(wreq.ifr_name, IFNAMSIZ, interface);
	wreq.u.essid.pointer = essid;
	wreq.u.essid.length = IW_ESSID_MAX_SIZE + 1;

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return;

	ioctl(sockfd, SIOCGIWESSID, &wreq);

	close(sockfd);
}

/*** newstr_link ***/
char * newstr_link(char *interface, unsigned int flags) {
	char *notifystr;
	char essid[IW_ESSID_MAX_SIZE + 1];

	memset(&essid, 0, IW_ESSID_MAX_SIZE + 1);
	get_ssid(interface, essid);

	if (strlen(essid) == 0) {
		notifystr = malloc(sizeof(TEXT_NEWLINK) + strlen(interface) + 4);
		sprintf(notifystr, TEXT_NEWLINK, interface, (flags & CHECK_CONNECTED) ? "up" : "down");
	} else {
		notifystr = malloc(sizeof(TEXT_WIRELESS) + strlen(interface) + 4 + strlen(essid));
		sprintf(notifystr, TEXT_WIRELESS, interface, (flags & CHECK_CONNECTED) ? "up" : "down", essid);
	}

	return notifystr;
}

/*** newstr_addr ***/
char * newstr_addr(char *interface, unsigned char family, char *ipaddr, unsigned char prefix) {
	char *notifystr;

	notifystr = malloc(sizeof(TEXT_NEWADDR)+ strlen(interface) + strlen(ipaddr));
	sprintf(notifystr, TEXT_NEWADDR, interface, family == AF_INET6 ? "IPv6" : "IP", ipaddr, prefix);

	return notifystr;
}

/*** newstr_away ***/
char * newstr_away(char *interface) {
	char *notifystr;

	notifystr = malloc(sizeof(TEXT_DELLINK) + strlen(interface));
	sprintf(notifystr, TEXT_DELLINK, interface);

	return notifystr;
}

/*** open_netlink ***/
int open_netlink (void) {
	int sock;
	struct sockaddr_nl addr;

	memset ((void *) &addr, 0, sizeof(addr));

	if ((sock = socket (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0)
		return sock;

	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0)
		return -1;

	return sock;
}

/*** read_event ***/
int read_event (int sockint) {
	int status, rc = EXIT_FAILURE;
	char buf[4096];
	struct iovec iov = { buf, sizeof buf };
	struct sockaddr_nl snl;
	struct msghdr msg = { (void *) &snl, sizeof snl, &iov, 1, NULL, 0, 0 };
	struct nlmsghdr *h;

	if ((status = recvmsg (sockint, &msg, 0)) < 0) {
		/* Socket non-blocking so bail out once we have read everything */
		if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINTR) {
			rc = EXIT_SUCCESS;
			goto out;
		}

		/* Anything else is an error */
		fprintf (stderr, "read_netlink: Error recvmsg: %d\n", status);
		goto out;
	}

	if (status == 0)
		fprintf (stderr, "read_netlink: EOF\n");

	/* We need to handle more than one message per 'recvmsg' */
	for (h = (struct nlmsghdr *) buf; NLMSG_OK (h, (unsigned int) status); h = NLMSG_NEXT (h, status)) {
		/* Finish reading */
		if (h->nlmsg_type == NLMSG_DONE) {
			rc = EXIT_SUCCESS;
			goto out;
		}

		/* Message is some kind of error */
		if (h->nlmsg_type == NLMSG_ERROR) {
			fprintf (stderr, "read_netlink: Message is an error - decode TBD\n");
			goto out;
		}

		/* Call message handler */
		if (msg_handler(&snl, h) != EXIT_SUCCESS) {
			fprintf (stderr, "read_event: Message hander returned error.\n");
			goto out;
		}
	}

	rc = EXIT_SUCCESS;

out:
	return rc;
}

/*** msg_handler ***/
int msg_handler (struct sockaddr_nl *nl, struct nlmsghdr *msg) {
	int rc = EXIT_FAILURE;
	char *notifystr = NULL;
	unsigned int errcount = 0;
	GError *error = NULL;
	struct ifaddrmsg *ifa;
	struct ifinfomsg *ifi;
	struct rtattr *rth;
	int rtl;
	char buf[INET6_ADDRSTRLEN];
	NotifyNotification *tmp_notification = NULL, *notification = NULL;
	char *icon = NULL;

	ifa = (struct ifaddrmsg *) NLMSG_DATA (msg);
	ifi = (struct ifinfomsg *) NLMSG_DATA (msg);

	/* make sure we have alloced memory for NotifyNotification and addresses_seen struct array */
	if (maxinterface < ifi->ifi_index) {
		ifs = realloc(ifs, (ifi->ifi_index + 1) * sizeof(struct ifs));
		while(maxinterface < ifi->ifi_index) {
			maxinterface++;

			if (verbose > 0)
				printf("%s: Initializing interface %d: ", program, maxinterface);

			/* get interface name and store it
			 * in case the interface does no longer exist this may fail,
			 * use static string '(unknown)' instead */
			if (if_indextoname(maxinterface, ifs[maxinterface].name) == NULL)
				strcpy(ifs[maxinterface].name, "(unknown)");

			if (verbose > 0)
				printf("%s\n", ifs[maxinterface].name);

			ifs[maxinterface].state = -1;
			ifs[maxinterface].deleted = 0;

			ifs[maxinterface].notification =
#				if NOTIFY_CHECK_VERSION(0, 7, 0)
				notify_notification_new(TEXT_TOPIC, NULL, NULL);
#				else
				notify_notification_new(TEXT_TOPIC, NULL, NULL, NULL);
#				endif
			notify_notification_set_category(ifs[maxinterface].notification, PROGNAME);
			notify_notification_set_urgency(ifs[maxinterface].notification, NOTIFY_URGENCY_NORMAL);
			notify_notification_set_timeout(ifs[maxinterface].notification, notification_timeout);

			ifs[maxinterface].addresses_seen = NULL;
		}
	} else if (ifs[ifi->ifi_index].deleted == 1) {
		if (verbose > 0)
			printf("%s: Ignoring event for deleted interface %d.\n", program, ifi->ifi_index);
		rc = EXIT_SUCCESS;
		goto out;
	}

	/* make notification point to the array element, will be overwritten
	 * later when needed for address notification */
	notification = ifs[ifi->ifi_index].notification;

	/* get interface name and store it
	 * in case the interface does no longer exist this may fail, but it does not overwrite */
	if_indextoname(ifi->ifi_index, ifs[ifi->ifi_index].name);

	if (verbose > 1)
		printf("%s: Event for interface %s (%d): flags = %x, msg type = %d\n",
			program, ifs[ifi->ifi_index].name, ifi->ifi_index, ifa->ifa_flags, msg->nlmsg_type);

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
					if (match_address(ifs[ifi->ifi_index].addresses_seen, buf, ifa->ifa_prefixlen)) {
						if (verbose > 0)
							printf("%s: Address %s/%d already known for %s, ignoring.\n",
									program, buf, ifa->ifa_prefixlen, ifs[ifi->ifi_index].name);
						break;
					}

					/* add address to struct */
					ifs[ifi->ifi_index].addresses_seen =
						add_address(ifs[ifi->ifi_index].addresses_seen, buf, ifa->ifa_prefixlen);
					if (verbose > 1)
						list_addresses(ifs[ifi->ifi_index].addresses_seen, ifs[ifi->ifi_index].name);

					/* display notification */
					notifystr = newstr_addr(ifs[ifi->ifi_index].name,
						ifa->ifa_family, buf, ifa->ifa_prefixlen);

					/* we are done, no need to run more loops */
					break;
				}
				rth = RTA_NEXT (rth, rtl);
			}
			/* we did not find anything to notify */
			if (notifystr == NULL) {
				rc = EXIT_SUCCESS;
				goto out;
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
			notify_notification_set_timeout(tmp_notification, notification_timeout);

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
					ifs[ifi->ifi_index].addresses_seen =
						remove_address(ifs[ifi->ifi_index].addresses_seen, buf, ifa->ifa_prefixlen);
					if (verbose > 1)
						list_addresses(ifs[ifi->ifi_index].addresses_seen, ifs[ifi->ifi_index].name);

					/* we are done, no need to run more loops */
					break;
				}
				rth = RTA_NEXT (rth, rtl);
			}

			rc = EXIT_SUCCESS;
			goto out;

		case RTM_NEWROUTE:
			rc = EXIT_SUCCESS;
			goto out;

		case RTM_DELROUTE:
			rc = EXIT_SUCCESS;
			goto out;

		case RTM_NEWLINK:
			/* ignore if state did not change */
			if ((ifi->ifi_flags & CHECK_CONNECTED) == ifs[ifi->ifi_index].state) {
				rc = EXIT_SUCCESS;
				goto out;
			}

			ifs[ifi->ifi_index].state = ifi->ifi_flags & CHECK_CONNECTED;

			notifystr = newstr_link(ifs[ifi->ifi_index].name, ifi->ifi_flags);

			icon = ifi->ifi_flags & CHECK_CONNECTED ? ICON_NETWORK_UP : ICON_NETWORK_DOWN;

			/* free only if interface goes down */
			if (!(ifi->ifi_flags & CHECK_CONNECTED)) {
				free_addresses(ifs[ifi->ifi_index].addresses_seen);
				ifs[ifi->ifi_index].addresses_seen = NULL;
			}

			break;
		case RTM_DELLINK:
			notifystr = newstr_away(ifs[ifi->ifi_index].name);

			icon = ICON_NETWORK_AWAY;

			free_addresses(ifs[ifi->ifi_index].addresses_seen);
			/* marking interface deleted makes events for this interface to be ignored */
			ifs[ifi->ifi_index].deleted = 1;

			break;
		default:
			/* we should not get here... */
			fprintf(stderr, "msg_handler: Unknown netlink nlmsg_type %d.\n", msg->nlmsg_type);

			goto out;
	}

	if (verbose > 0)
		printf("%s: %s\n", program, notifystr);

	notify_notification_update(notification, TEXT_TOPIC, notifystr, icon);

	while (notify_notification_show(notification, &error) == FALSE) {
		if (errcount > 1) {
			fprintf(stderr, "%s: Looks like we can not reconnect to notification daemon... Exiting.\n", program);
			goto out;
		} else {
			g_printerr("%s: Error \"%s\" while trying to show notification. Trying to reconnect.\n", program, error->message);
			errcount++;

			g_error_free(error);
			error = NULL;

			notify_uninit();

			usleep (500 * 1000);

			if (notify_init(PROGNAME) == FALSE) {
				fprintf(stderr, "%s: Can't create notify.\n", program);
				goto out;
			}
		}
	}

	rc = EXIT_SUCCESS;

out:
	if (tmp_notification)
		g_object_unref(G_OBJECT(tmp_notification));
	errcount = 0;
	free(notifystr);

	return rc;
}

/*** received_signal ***/
void received_signal(int signal) {
	if (verbose > 0)
		printf("%s: Received signal: %s\n", program, strsignal(signal));

	doexit++;
}

/*** main ***/
int main (int argc, char **argv) {
	int rc = EXIT_FAILURE;
	int i, nls;
	unsigned int version = 0, help = 0;

	program = argv[0];

	/* get the verbose status */
	while ((i = getopt_long(argc, argv, optstring, options_long, NULL)) != -1) {
		switch (i) {
			case 'h':
				help++;
				break;
			case 't':
				notification_timeout = atof(optarg) * 1000;
				break;
			case 'v':
				verbose++;
				break;
			case 'V':
				verbose++;
				version++;
				break;
		}
	}

	if (verbose > 0)
		printf ("%s: %s v%s"
#ifdef HAVE_SYSTEMD
			" +systemd"
#endif
			" (compiled: " __DATE__ ", " __TIME__ ")\n", program, PROGNAME, VERSION);

	if (help > 0)
		printf("usage: %s [-h] [-t TIMEOUT] [-v[v]] [-V]\n", program);

	if (version > 0 || help > 0)
		return EXIT_SUCCESS;

	if ((nls = open_netlink()) < 0) {
		fprintf (stderr, "%s: Error opening netlink socket!\n", program);
		goto out40;
	}

	if (notify_init(PROGNAME) == FALSE) {
		fprintf (stderr, "%s: Can't create notify.\n", program);
		goto out30;
	}

	signal(SIGINT, received_signal);
	signal(SIGTERM, received_signal);

#ifdef HAVE_SYSTEMD
	sd_notify(0, "READY=1\nSTATUS=Waiting for netlink events...");
#endif

	while (doexit == 0) {
		if (read_event(nls) != EXIT_SUCCESS) {
			fprintf(stderr, "%s: read_event returned error.\n", program);
			goto out10;
		}
	}

	if (verbose > 0)
		printf("%s: Exiting...\n", program);

	/* report stopping to systemd */
#ifdef HAVE_SYSTEMD
	sd_notify(0, "STOPPING=1\nSTATUS=Stopping...");
#endif

	for(; maxinterface > 0; maxinterface--) {
		if (verbose > 0)
			printf("%s: Freeing interface %d: %s\n", program,
					maxinterface, ifs[maxinterface].name);

		free_addresses(ifs[maxinterface].addresses_seen);
		if (ifs[maxinterface].notification != NULL)
			g_object_unref(G_OBJECT(ifs[maxinterface].notification));
	}

	rc = EXIT_SUCCESS;

out10:
	if (ifs != NULL)
		free(ifs);

/* out20: */
	notify_uninit();

out30:
	if (close(nls) < 0)
		fprintf(stderr, "%s: Failed to close socket.\n", program);

out40:
#ifdef HAVE_SYSTEMD
	sd_notify(0, "STATUS=Stopped. Bye!");
#endif

	return rc;
}
