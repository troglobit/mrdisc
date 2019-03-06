/* Shared code between mrdisc and solicit
 *
 * Copyright (c) 2017-2019  Joachim Nilsson <troglobit@gmail.com>
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

#include <arpa/inet.h>
#include <err.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint16_t in_cksum(uint16_t *p, size_t len)
{
	uint32_t sum = 0;
	size_t i;

	for (i = 0; i < len; i++)
		sum += ntohs(p[i]);

	sum = (sum % 0x10000) + (sum / 0x10000);

	return htons(((uint16_t)sum) ^ 0xffff);
}

void compose_addr6(struct sockaddr_in6 *sin, char *group)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin6_family = AF_INET6;

	if (!inet_pton(AF_INET6, group, &sin->sin6_addr))
		err(1, "Failed preparing %s", group);
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
