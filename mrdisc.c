/* mrdisc daemon
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

#include <config.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <sys/socket.h>

#define IGMP_MRDISC_ANNOUNCE 0x30
#define IGMP_MRDISC_SOLICIT  0x31
#define IGMP_MRDISC_TERM     0x32
#define MC_ALL_SNOOPERS      "224.0.0.106"

typedef struct {
	int   sd;
	char *ifname;
} ifsock_t;

int      running = 1;
size_t   ifnum = 0;
ifsock_t iflist[100];

unsigned short in_cksum(unsigned short *addr, int len);


static void open_socket(char *ifname)
{
	char loop;
	int sd, val, rc;
	struct ifreq ifr;
	unsigned char ra[4] = { IPOPT_RA, 0x04, 0x00, 0x00 };

	sd = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
	if (sd < 0)
		err(1, "Cannot open socket");

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
	if (setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
		if (ENODEV == errno) {
			warnx("Not a valid interface, %s, skipping ...", ifname);
			close(sd);
			return;
		}

		err(1, "Cannot bind socket to interface %s", ifname);
	}

	val = 1;
	rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &val, sizeof(val));
	if (rc < 0)
		err(1, "Cannot set TTL");

	loop = 0;
	rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
	if (rc < 0)
		err(1, "Cannot disable MC loop");

	rc = setsockopt(sd, IPPROTO_IP, IP_OPTIONS, &ra, sizeof(ra));
	if (rc < 0)
		err(1, "Cannot set IP OPTIONS");

	iflist[ifnum].sd = sd;
	iflist[ifnum].ifname = ifname;
	ifnum++;
}

static int close_socket(void)
{
	size_t i;
	int ret = 0;

	for (i = 0; i < ifnum; i++)
		ret |= close(iflist[i].sd);

	return ret;
}

static void compose_addr(struct sockaddr_in *sin, char *group)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin_family      = AF_INET;
	sin->sin_addr.s_addr = inet_addr(group);
}

static void send_message(uint8_t type, uint8_t interval)
{
	size_t i;
	struct igmp igmp;
	struct sockaddr dest;

	memset(&igmp, 0, sizeof(igmp));
	igmp.igmp_type = type;
	igmp.igmp_code = interval;
	igmp.igmp_cksum = in_cksum((unsigned short *)&igmp, sizeof(igmp));

	compose_addr((struct sockaddr_in *)&dest, MC_ALL_SNOOPERS);

	for (i = 0; i < ifnum; i++) {
		ssize_t num;

		num = sendto(iflist[i].sd, &igmp, sizeof(igmp), 0, &dest, sizeof(dest));
		if (num < 0)
			err(1, "Cannot send Membership report message.");
	}
}

static void exit_handler(int signo)
{
	running = 0;
}

static void signal_init(void)
{
	signal(SIGTERM, exit_handler);
	signal(SIGINT,  exit_handler);
	signal(SIGHUP,  exit_handler);
	signal(SIGQUIT, exit_handler);
}

static int usage(int code)
{
	printf("\nUsage: %s [-i SEC] IFACE [IFACE ...]\n"
	       "\n"
	       "    -h        This help text\n"
	       "    -i SEC    Announce interval, 4-180 sec, default 20 sec\n"
	       "\n"
	       "Bug report address: %-40s\n\n", PACKAGE_NAME, PACKAGE_BUGREPORT);

	return code;
}

int main(int argc, char *argv[])
{
	int i, c;
	int interval = 20;

	while ((c = getopt(argc, argv, "hi:")) != EOF) {
		switch (c) {
		case 'h':
			return usage(0);

		case 'i':
			interval = atoi(optarg);
			if (interval < 4 || interval > 180)
				errx(1, "Invalid announcement interval [4,180]");
			break;

		default:
			break;
		}
	}

	if (optind >= argc) {
		warnx("Not enough arguments");
		return usage(1);
	}

	signal_init();

	for (i = optind; i < argc; i++)
		open_socket(argv[i]);

	while (running) {
		send_message(IGMP_MRDISC_ANNOUNCE, interval);
		sleep(interval);
	}

	send_message(IGMP_MRDISC_TERM, 0);

	return close_socket();
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
