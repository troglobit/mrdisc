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
#include <sys/socket.h>
#include <time.h>

#include "if.h"
#include "inet.h"

size_t   ifnum4 = 0;
size_t   ifnum6 = 0;
ifsock_t iflist4[MAX_NUM_IFACES];
ifsock_t iflist6[MAX_NUM_IFACES];

void if_init4(char *iface[], int num)
{
	int i, sd;
	char *ifname;

	for (i = 0; i < num; i++) {
		ifname = iface[i];

		sd = inet_open(ifname);
		if (sd < 0)
			continue;

		iflist4[ifnum4].sd = sd;
		iflist4[ifnum4].ifname = ifname;

		ifnum4++;
	}
}

void if_init6(char *iface[], int num)
{
	int i, sd;
	char *ifname;

	for (i = 0; i < num; i++) {
		ifname = iface[i];

		sd = inet6_open(ifname);
		if (sd < 0)
			continue;

		iflist6[ifnum6].sd = sd;
		iflist6[ifnum6].ifname = ifname;

		ifnum6++;
	}
}

int if_exit(void)
{
	size_t i;
	int ret = 0;

	for (i = 0; i < ifnum4; i++)
		ret |= inet_close(iflist4[i].sd);
	for (i = 0; i < ifnum6; i++)
		ret |= inet6_close(iflist6[i].sd);

	return ret;
}

void if_send4(uint8_t interval)
{
	size_t i;

	for (i = 0; i < ifnum4; i++) {
		if (inet_send(iflist4[i].sd, IGMP_MRDISC_ANNOUNCE, interval))
			warn("Failed sending IGMP control message 0x%x on %s",
			     IGMP_MRDISC_ANNOUNCE, iflist4[i].ifname);
	}
}

void if_send6(uint8_t interval)
{
	size_t i;

	for (i = 0; i < ifnum6; i++) {
		if (inet6_send(iflist6[i].sd, ICMP6_MRDISC_ANNOUNCE, interval))
			warn("Failed sending ICMPv6 control message 0x%x on %s",
			     ICMP6_MRDISC_ANNOUNCE, iflist6[i].ifname);
	}
}

static void if_poll_init(const ifsock_t sd_arr[], struct pollfd pfd[], int npfd)
{
	int i;

	for (i = 0; i < npfd; i++) {
		pfd[i].fd = sd_arr[i].sd;
		pfd[i].events = POLLIN | POLLPRI | POLLHUP;
	}
}

static int if_recv(int sd, int af, uint8_t interval)
{
	switch (af) {
	case AF_INET:
		return inet_recv(sd, interval);
	case AF_INET6:
		return inet6_recv(sd, interval);
	default:
		return -1;
	}
}

static int if_poll_recv(int af, const ifsock_t sd_arr[],
			const struct pollfd pfd[],
			int npfd, int npoll, uint8_t interval)
{
	int i;

	for (i = 0; npoll > 0 && i < npfd; i++) {
		int   sd     = sd_arr[i].sd;
		char *ifname = sd_arr[i].ifname;

		if (pfd[i].revents & POLLIN) {
			if (if_recv(sd, af, interval))
				warn("Failed reading from interface %s", ifname);
			npoll--;
		}
	}

	return npoll;
}

void if_poll(uint8_t interval)
{
	size_t i;
	int num;
	time_t end = time(NULL) + interval;
	struct pollfd pfd[MAX_NUM_IFACES*2];

	if_poll_init(iflist4, &pfd[0], ifnum4);
	if_poll_init(iflist6, &pfd[ifnum4], ifnum6);

	while (1) {
		num = poll(pfd, ifnum4 + ifnum6, (end - time(NULL)) * 1000);
		if (num < 0) {
			if (EINTR == errno)
				break;

			err(1, "Unrecoverable error");
		}

		if (num == 0)
			break;

		num = if_poll_recv(AF_INET, iflist4, &pfd[0], ifnum4, num, interval);
		num = if_poll_recv(AF_INET6, iflist6, &pfd[ifnum4], ifnum6, num, interval);
	}
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
