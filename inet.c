/* IPv4 backend
 *
 * Copyright (c) 2017-2021  Joachim Wiberg <troglobit@gmail.com>
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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/igmp.h>
#include <netinet/icmp6.h>
#include <sys/socket.h>

#include "inet.h"

#define MC_ALL_ROUTERS       "224.0.0.2"
#define MC6_ALL_ROUTERS      "ff02::2"
#define MC_ALL_SNOOPERS      "224.0.0.106"
#define MC6_ALL_SNOOPERS     "ff02::6a"

uint16_t in_cksum(uint16_t *p, size_t len);
void compose_addr6(struct sockaddr_in6 *sin, char *group);

int inet_open(char *ifname)
{
	char loop;
	int sd, val, rc;
	struct ifreq ifr;
	struct ip_mreqn mreq;
	unsigned char ra[4] = { IPOPT_RA, 0x04, 0x00, 0x00 };

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

	memset(&mreq, 0, sizeof(mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(MC_ALL_ROUTERS);
	mreq.imr_ifindex = if_nametoindex(ifname);
        if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)))
		err(1, "Failed joining group %s", MC_ALL_ROUTERS);

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

	return sd;
}

int inet6_open(char *ifname)
{
	int loop = 0;
	int sd, hops = 1, rc;
	struct ifreq ifr;
	struct ipv6_mreq mreq;

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

	memset(&mreq, 0, sizeof(mreq));
	mreq.ipv6mr_interface = if_nametoindex(ifname);

	if(!inet_pton(AF_INET6, MC6_ALL_ROUTERS, &mreq.ipv6mr_multiaddr))
		err(1, "Failed preparing %s", MC6_ALL_ROUTERS);

	if (setsockopt(sd, IPPROTO_IPV6, IPV6_JOIN_GROUP, (void *) &mreq, sizeof(mreq)))
		err(1, "Failed joining group %s", MC6_ALL_ROUTERS);

	rc = setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &hops, sizeof(hops));
	if (rc < 0)
		err(1, "Cannot set hop limit");

	rc = setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop));
	if (rc < 0)
		err(1, "Cannot disable MC loop");

	rc = setsockopt(sd, IPPROTO_IPV6, IPV6_HOPOPTS, &hopopt, sizeof(hopopt));
	if (rc < 0)
		err(1, "Cannot set IPV6 hop-by-hop option");

	return sd;
}

int inet_close(int sd)
{
	return  inet_send(sd, IGMP_MRDISC_TERM, 0) ||
		close(sd);
}

int inet6_close(int sd)
{
	return  inet6_send(sd, ICMP6_MRDISC_TERM, 0) ||
		close(sd);
}

static void compose_addr(struct sockaddr_in *sin, char *group)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin_family      = AF_INET;
	sin->sin_addr.s_addr = inet_addr(group);
}

int inet_send(int sd, uint8_t type, uint8_t interval)
{
	ssize_t num;
	struct igmp igmp;
	struct sockaddr dest;

	memset(&igmp, 0, sizeof(igmp));
	igmp.igmp_type = type;
	igmp.igmp_code = interval;
	igmp.igmp_cksum = in_cksum((uint16_t *)&igmp, sizeof(igmp) / 2);

	compose_addr((struct sockaddr_in *)&dest, MC_ALL_SNOOPERS);

	num = sendto(sd, &igmp, sizeof(igmp), 0, &dest, sizeof(dest));
	if (num < 0)
		return 1;

	return 0;
}

int inet6_send(int sd, uint8_t type, uint8_t interval)
{
	ssize_t num;
	struct icmp6_hdr icmp6;
	struct sockaddr_in6 dest;

	memset(&icmp6, 0, sizeof(icmp6));
	icmp6.icmp6_type = type;
	icmp6.icmp6_code = interval;
	icmp6.icmp6_cksum = 0; /* updated by kernel */

	compose_addr6(&dest, MC6_ALL_SNOOPERS);

	num = sendto(sd, &icmp6, sizeof(icmp6), 0, (struct sockaddr *)&dest,
		     sizeof(dest));
	if (num < 0)
		return 1;

	return 0;
}

int inet_recv(int sd, uint8_t interval)
{
	char buf[1530];
	ssize_t num;
	struct ip *ip;
	struct igmp *igmp;

	memset(buf, 0, sizeof(buf));
	num = read(sd, buf, sizeof(buf));
	if (num < 0)
		return -1;

	ip = (struct ip *)buf;
	igmp = (struct igmp *)(buf + (ip->ip_hl << 2));
	if (igmp->igmp_type == IGMP_MRDISC_SOLICIT)
		return inet_send(sd, IGMP_MRDISC_ANNOUNCE, interval);

	return 0;
}

int inet6_recv(int sd, uint8_t interval)
{
	char buf[1530];
	ssize_t num;
	struct icmp6_hdr *icmp6;

	memset(buf, 0, sizeof(buf));
	num = read(sd, buf, sizeof(buf));
	if (num < (ssize_t)(sizeof(*icmp6)))
		return -1;

	icmp6 = (struct icmp6_hdr *)buf;
	if (icmp6->icmp6_type == ICMP6_MRDISC_SOLICIT)
		return inet6_send(sd, ICMP6_MRDISC_ANNOUNCE, interval);

	return 0;
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
