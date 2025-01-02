/*
 * (C) 2011-2025 by Christian Hesse <mail@eworm.de>
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
#define TEXT_WIRELESS	"Interface <b>%s</b> is <b>%s</b> on <b>%s</b>."
#define TEXT_NEWADDR	"Interface <b>%s</b> has new %s address\n<b>%s</b>/%d."
#define TEXT_DELLINK	"Interface <b>%s</b> has gone away."

#endif /* CONFIG_H */
