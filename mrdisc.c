/* mrdisc daemon
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

#include <config.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "if.h"

int      running = 1;
uint8_t  interval = 20;
char     version_info[] = PACKAGE_NAME " v" PACKAGE_VERSION;


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
	printf("\nUsage: %s [-4|-6] [-i SEC] IFACE [IFACE ...]\n"
	       "\n"
	       "    -h        This help text\n"
	       "    -4        Use IPv4 only\n"
	       "    -6        Use IPv6 only\n"
	       "    -i SEC    Announce interval, 4-180 sec, default 20 sec\n"
	       "    -v        Program version\n"
	       "\n"
	       "Bug report address: %-40s\n\n", PACKAGE_NAME, PACKAGE_BUGREPORT);

	return code;
}

int main(int argc, char *argv[])
{
	int v4 = 1;
	int v6 = 1;
	int c;
	int ret;

	while ((c = getopt(argc, argv, "hi:v46")) != EOF) {
		switch (c) {
		case 'h':
			return usage(0);

		case 'i':
			interval = (uint8_t)atoi(optarg);
			if (interval < 4 || interval > 180)
				errx(1, "Invalid announcement interval [4,180]");
			break;

		case 'v':
			fprintf(stderr, "%s\n", version_info);
			return 0;
		case '4':
			v6 = 0;
			break;
		case '6':
			v4 = 0;
			break;

		default:
			return usage(1);
		}
	}

	if (!v4 && !v6) {
		warnx("-4 and -6 are exclusive");
		return usage(1);
	}

	if (optind >= argc) {
		warnx("Not enough arguments");
		return usage(1);
	}

	signal_init();

	if (v4)
		if_init4(&argv[optind], argc - optind);
	if (v6)
		if_init6(&argv[optind], argc - optind);

	while (running) {
		if (v4)
			if_send4(interval);
		if (v6)
			if_send6(interval);

		if_poll(interval);
	}

	return if_exit();
}

/**
 * Local Variables:
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
