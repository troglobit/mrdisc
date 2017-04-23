/* Interface handling
 *
 * Copyright (c) 2017  Joachim Nilsson <troglobit@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "if.h"
#include "inet.h"

size_t   ifnum = 0;
ifsock_t iflist[MAX_NUM_IFACES];

void if_init(char *iface[], int num)
{
	int i, sd;
	char *ifname;

	for (i = 0; i < num; i++) {
		ifname = iface[i];

		sd = inet_open(ifname);
		if (sd < 0)
			continue;

		iflist[ifnum].sd = sd;
		iflist[ifnum].ifname = ifname;
		ifnum++;
	}
}

int if_exit(void)
{
	size_t i;
	int ret = 0;

	for (i = 0; i < ifnum; i++)
		ret |= inet_close(iflist[i].sd);

	return ret;
}

void if_send(uint8_t interval)
{
	size_t i;

	for (i = 0; i < ifnum; i++) {
		if (inet_send(iflist[i].sd, IGMP_MRDISC_ANNOUNCE, interval))
			warn("Failed sending IGMP control message 0x%x on %s",
			     IGMP_MRDISC_ANNOUNCE, iflist[i].ifname);
	}
}

void if_poll(uint8_t interval)
{
	size_t i;
	int num = 1;
	time_t end = time(NULL) + interval;
	struct pollfd pfd[MAX_NUM_IFACES];

	for (i = 0; i < ifnum; i++) {
		pfd[i].fd = iflist[i].sd;
		pfd[i].events = POLLIN | POLLPRI | POLLHUP;
	}

again:
	while (1) {
		num = poll(pfd, ifnum, (end - time(NULL)) * 1000);
		if (num < 0) {
			if (EINTR == errno)
				break;

			err(1, "Unrecoverable error");
		}

		if (num == 0)
			break;

		for (i = 0; num > 0 && i < ifnum; i++) {
			int   sd     = iflist[i].sd;
			char *ifname = iflist[i].ifname;

			if (pfd[i].revents & POLLIN) {
				if (inet_recv(sd, interval))
					warn("Failed reading from interface %s", ifname);
				num--;
			}
		}
	}
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
