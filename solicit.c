/* Simple test mrdisc solicitation agent
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/igmp.h>
#include <netinet/icmp6.h>
#include <sys/socket.h>

#define MC_ALL_ROUTERS       "224.0.0.2"
#define MC6_ALL_ROUTERS      "ff02::2"
#define IGMP_MRDISC_ANNOUNCE 0x30
#define IGMP_MRDISC_SOLICIT  0x31
#define IGMP_MRDISC_TERM     0x32
#define ICMP6_MRDISC_SOLICIT 152

uint16_t in_cksum(uint16_t *p, size_t len);
void compose_addr6(struct sockaddr_in6 *sin, char *group);

static int open_socket(char *ifname)
{
	unsigned char ra[4] = { IPOPT_RA, 0x04, 0x00, 0x00 };
	struct ifreq ifr;
	char loop;
	int val = 1;
	int sd, rc;

	sd = socket(AF_INET, SOCK_RAW | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_IGMP);
	if (sd < 0)
		err(1, "Cannot open socket");

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
	if (setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
		if (ENODEV == errno) {
			warnx("Not a valid interface, %s, skipping ...", ifname);
			close(sd);
			return -1;
		}

		err(1, "Cannot bind socket to interface %s", ifname);
	}

	rc = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &val, sizeof(val));
	if (rc < 0)
		err(1, "Cannot set TTL");

	rc = setsockopt(sd, IPPROTO_IP, IP_OPTIONS, &ra, sizeof(ra));
	if (rc < 0)
		err(1, "Cannot set IP OPTIONS");

	return sd;
}

static int open_socket6(char *ifname)
{
	int loop = 0;
	int sd, hops = 1, rc;
	struct ifreq ifr;

	/**
	 * hopopt[8]:
	 * {
	 *   "nexthdr": 0x00 (updated by kernel), "hdrextlen": 0x00,
	 *   "rtalert": {
	 *     "type": 0x05, "length": 0x00, "value": [ 0x00, 0x00 ] },
	 *   }
	 *   "PadN": [ 0x01, 0x00 ]
	 * }
	 */
	unsigned char hopopt[8] = { 0x00, 0x00, 0x05, 0x02, 0x00, 0x00, 0x01, 0x00 };

	sd = socket(AF_INET6, SOCK_RAW | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_ICMPV6);
	if (sd < 0)
		err(1, "Cannot open socket");

	memset(&ifr, 0, sizeof(ifr));
	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", ifname);
	if (setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
		if (ENODEV == errno) {
			warnx("Not a valid interface, %s, skipping ...", ifname);
			close(sd);
			return -1;
		}

		err(1, "Cannot bind socket to interface %s", ifname);
	}

	rc = setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops));
	if (rc < 0)
		err(1, "Cannot set hop limit");

	rc = setsockopt(sd, IPPROTO_IPV6, IPV6_HOPOPTS, &hopopt, sizeof(hopopt));
	if (rc < 0)
		err(1, "Cannot set IPV6 hop-by-hop option");

	return sd;

}

static void compose_addr(struct sockaddr_in *sin, char *group)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin_family      = AF_INET;
	sin->sin_addr.s_addr = inet_addr(group);
}

static int send_message(int sd, uint8_t type, uint8_t interval)
{
	struct sockaddr dest;
	struct igmp igmp;
	ssize_t num;

	memset(&igmp, 0, sizeof(igmp));
	igmp.igmp_type = type;
	igmp.igmp_code = interval;
	igmp.igmp_cksum = in_cksum((uint16_t *)&igmp, sizeof(igmp) / 2);

	compose_addr((struct sockaddr_in *)&dest, MC_ALL_ROUTERS);

	num = sendto(sd, &igmp, sizeof(igmp), 0, &dest, sizeof(dest));
	if (num < 0)
		err(1, "Cannot send IGMP control message");

	return 0;
}

static int send_message6(int sd, uint8_t type, uint8_t interval)
{
	ssize_t num;
	struct icmp6_hdr icmp6;
	struct sockaddr_in6 dest;

	memset(&icmp6, 0, sizeof(icmp6));
	icmp6.icmp6_type = type;
	icmp6.icmp6_code = interval;
	icmp6.icmp6_cksum = 0; /* updated by kernel */

	compose_addr6(&dest, MC6_ALL_ROUTERS);

	num = sendto(sd, &icmp6, sizeof(icmp6), 0, (struct sockaddr *)&dest,
		     sizeof(dest));
	if (num < 0)
		return 1;

	return 0;
}

int main(int argc, char *argv[])
{
	int v4 = 1;
	int v6 = 1;
	char *ifname;
	int sd, ret;

	if (argc < 2)
		errx(1, "Usage: %s [-4|-6] IFNAME", argv[0]);

	if (!strncmp(argv[1], "-4", strlen("-4"))) {
		v6 = 0;
		ifname = argv[2];
	} else if (!strncmp(argv[1], "-6", strlen("-6"))) {
		v4 = 0;
		ifname = argv[2];
	} else {
		ifname = argv[1];
	}

	if ((!v4 || !v6) && argc < 3)
		errx(1, "Usage: %s [-4|-6] IFNAME", argv[0]);

	if (v4) {
		sd = open_socket(ifname);
		if (sd < 0)
			return 1;

		ret = send_message(sd, IGMP_MRDISC_SOLICIT, 0);
		close(sd);
	}

	if (v6) {
		sd = open_socket6(ifname);
		if (sd < 0)
			return 1;

		ret |= send_message6(sd, ICMP6_MRDISC_SOLICIT, 0);
		close(sd);
	}

	return ret;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
