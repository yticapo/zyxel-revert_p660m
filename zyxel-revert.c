#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <getopt.h>

#include "configfile.h"
#include "context.h"
#include "event.h"
#include "serial.h"
#include "xmodem.h"

#define DEFAULT_CONFIG "zyxel-revert.conf"

static struct option opts[] = {
	{"config",	1, 0, 'c'},
	{"debug",	0, 0, 'd'},
	{"help",	0, 0, 'h'},
	{0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
	char *config = DEFAULT_CONFIG;
	int code, arg = 0, debug = 0;

	do {
		code = getopt_long(argc, argv, "c:dh", opts, &arg);

		switch (code) {
		case 'c':	/* config */
				config = optarg;
				break;

		case 'd':	/* debug */
				debug = 1;
				break;

		case 'h':	/* help */
				printf("Usage: zyxel-revert [options]\n"
					"Options: \n"
					"  --config       -c  configfile  use this configfile\n"
					"  --debug        -d              do not fork and log to stderr\n"
					"  --help         -h              this help\n"
					"\n");
				exit(0);
				break;

		case '?':	/* error */
				exit(-1);
				break;

		default:	/* unknown / all options parsed */
				break;
		}
	} while (code != -1);

	if (config_parse(config))
		exit(1);

	if (serial_init())
		exit(1);

//	if (network_init()) {
//		context_close();
//		exit(1);
//	}

	if (xmodem_init()) {
		context_close();
		exit(1);
	}

	event_loop();

	xmodem_close();
	context_close();

	return 0;
}
