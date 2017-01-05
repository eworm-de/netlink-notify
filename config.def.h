/*
 * (C) 2011-2017 by Christian Hesse <mail@eworm.de>
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 */

#ifndef CONFIG_H
#define CONFIG_H

/* how long to show notifications */
#define NOTIFICATION_TIMEOUT	10000

/* define icons */
#define ICON_NETWORK_ADDRESS	"netlink-notify-address"
#define ICON_NETWORK_UP		"netlink-notify-up"
#define ICON_NETWORK_DOWN	"netlink-notify-down"
#define ICON_NETWORK_AWAY	"netlink-notify-away"

/* define notification text messages */
#define TEXT_TOPIC	"Netlink Notification"
#define TEXT_NEWLINK	"Interface <b>%s</b> is <b>%s</b>."
#define TEXT_NEWADDR	"Interface <b>%s</b> has new %s address\n<b>%s</b>/%d."
#define TEXT_DELLINK	"Interface <b>%s</b> has gone away."

#endif /* CONFIG_H */
