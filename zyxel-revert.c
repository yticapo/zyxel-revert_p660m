#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <getopt.h>

#include "context.h"
#include "event.h"
#include "serial.h"

static struct option opts[] = {
	{"config",	1, 0, 'c'},
	{"device",	1, 0, 'd'},
	{"firmware",	1, 0, 'f'},
	{"slow",	0, 0, 's'},
	{0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
	char *devicename = NULL, *filename = NULL;
	int code, arg = 0, flags = FLAG_SPEEDUP;

	do {
		code = getopt_long(argc, argv, "c:d:f:s", opts, &arg);

		switch (code) {
		case 'c':	filename = optarg;
				flags |= FLAG_CONFIG;
				break;

		case 'd':	devicename = optarg;
				break;

		case 'f':	filename = optarg;
				flags |= FLAG_FIRMWARE;
				break;

		case 'h':	/* help */
				printf("Usage: zyxel-revert -d <device> [-s] [ -f <file> | -c <file> ]\n");
				exit(0);
				break;

		case 's':	flags &= ~FLAG_SPEEDUP;
				break;

		case '?':	/* error */
				exit(-1);
				break;

		default:	/* unknown / all options parsed */
				break;
		}
	} while (code != -1);

	if (devicename == NULL || filename == NULL)
		return -1;

	struct context *ctx = create_context(filename);
	if (ctx == NULL)
		exit(1);

	ctx->flags = flags;

	if (serial_init(ctx, devicename)) {
		context_close();
		exit(1);
	}

	event_loop();

	context_close();
	return 0;
}
