netlink-notify
==============

[![GitHub stars](https://img.shields.io/github/stars/eworm-de/netlink-notify?logo=GitHub&style=flat&color=red)](https://github.com/eworm-de/netlink-notify/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/eworm-de/netlink-notify?logo=GitHub&style=flat&color=green)](https://github.com/eworm-de/netlink-notify/network)
[![GitHub watchers](https://img.shields.io/github/watchers/eworm-de/netlink-notify?logo=GitHub&style=flat&color=blue)](https://github.com/eworm-de/netlink-notify/watchers)

**Notify about netlink changes**

This runs in background and produces notifications whenever a netlink event
is received. Notifications look like this:

A device is down:

![Device down](screenshots/down.png)

A device is up:

![Device up](screenshots/up.png)

A device received an IP address:

![Device received IP address](screenshots/ip.png)

A device received an IPv6 address:

![Device received IPv6 address](screenshots/ipv6.png)

A device disappeared:

![Device disappeared](screenshots/away.png)

*Use at your own risk*, pay attention to
[license and warranty](#license-and-warranty), and
[disclaimer on external links](#disclaimer-on-external-links)!

Requirements
------------

To compile and run `netlink-notify` you need:

* [systemd ↗️](https://www.github.com/systemd/systemd)
* [libnotify ↗️](https://developer.gnome.org/notification-spec/)
* [linux ↗️](https://www.kernel.org/)
* [markdown ↗️](https://daringfireball.net/projects/markdown/) (HTML documentation)
* [librsvg ↗️](https://wiki.gnome.org/Projects/LibRsvg) (convert icons from SVG to PNG)
* [oxipng ↗️](https://github.com/shssoichiro/oxipng) (optimize PNG icons)

Some systems may require additional development packages for the libraries.
Look for `libnotify-devel` or similar.

Build and install
-----------------

Building and installing is very easy. Just run:

    make

followed by:

    make install

This will place an executable at `/usr/bin/netlink-notify`,
documentation can be found in `/usr/share/doc/netlink-notify/`.
Additionally a systemd unit file is installed to `/usr/lib/systemd/user/`.

Usage
-----

Just run `netlink-notify` to run it once. A systemd user service can be
started and/or enabled with `systemctl --user start netlink-notify`
or `systemctl --user enable netlink-notify`.

License and warranty
--------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
[GNU General Public License](COPYING.md) for more details.

Disclaimer on external links
----------------------------

Our website contains links to the websites of third parties ("external
links"). As the content of these websites is not under our control, we
cannot assume any liability for such external content. In all cases, the
provider of information of the linked websites is liable for the content
and accuracy of the information provided. At the point in time when the
links were placed, no infringements of the law were recognisable to us.
As soon as an infringement of the law becomes known to us, we will
immediately remove the link in question.

> 💡️ **Hint**: All external links are marked with an arrow pointing
> diagonally in an up-right (or north-east) direction (↗️).

### Upstream

URL:
[GitHub.com](https://github.com/eworm-de/netlink-notify#netlink-notify)

Mirror:
[eworm.de](https://git.eworm.de/cgit.cgi/netlink-notify/)
[GitLab.com](https://gitlab.com/eworm-de/netlink-notify#netlink-notify)

---
[⬆️ Go back to top](#top)
