#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <getopt.h>

#include "context.h"
#include "event.h"
#include "serial.h"
#include "xmodem.h"

static struct option opts[] = {
	{"device",	1, 0, 'd'},
	{"device",	1, 0, 'f'},
	{0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
	char *devicename = NULL, *filename = NULL;
	int code, arg = 0;

	do {
		code = getopt_long(argc, argv, "d:f:", opts, &arg);

		switch (code) {
		case 'd':	devicename = optarg;
				break;

		case 'f':	filename = optarg;
				break;

		case 'h':	/* help */
				printf("Usage: zyxel-revert -d <device> -f <file>\n");
				exit(0);
				break;

		case '?':	/* error */
				exit(-1);
				break;

		default:	/* unknown / all options parsed */
				break;
		}
	} while (code != -1);

	if (devicename == NULL || serial_init(devicename))
		exit(1);

	if (filename == NULL || xmodem_init(filename)) {
		context_close();
		exit(1);
	}

	event_loop();

	xmodem_close();
	context_close();
	return 0;
}
