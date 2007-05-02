#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <termios.h>

#include "context.h"
#include "linebuffer.h"
#include "logging.h"
#include "xmodem.h"

/* states */
enum {
	STATE_NONE = 0,
	STATE_DEBUG_ASK,	/* waiting for "Press any key.." */
	STATE_DEBUG,		/* debug mode entered */
	STATE_SWITCH_BAUDRATE,	/* switching baudrate */
	STATE_XMODEM,		/* xmodem active */
	STATE_XMODEM_COMPLETE,	/* xmodem complete */
	STATE_BOOT,		/* booting firmware */
};

/* messages */
char *rx_msg[] = {
	"Press any key to enter debug mode within 3 seconds.",
	"Enter Debug Mode",
	"Now, console speed will be changed to 115200 bps",
	"OK",
	"Console speed will be changed to 9600 bps",
	NULL
};

/* message IDs */
enum {
	MSG_DEBUG_ASK = 0,
	MSG_DEBUG,
	MSG_BAUD_HIGH,
	MSG_XMODEM_OK,
	MSG_BAUD_LOW,
};

int statemachine_read(int fd, void *privdata)
{
	struct context *ctx = (struct context *)privdata;

	if (ctx->state == STATE_XMODEM) {
		if (xmodem_read(fd, privdata) < 0)
			ctx->state = STATE_XMODEM_COMPLETE;

		return 0;
	}

	if (linebuffer_read(ctx->lbuf, fd) < 0)
		return -1;

	char *line;
	while ((line = linebuffer_get(ctx->lbuf)) != NULL) {

		log_print(LOG_DEBUG, "%s: '%s'", ctx->devname, line);

		int msg = 0;
		while (rx_msg[msg] != NULL) {
			if (strcmp(line, rx_msg[msg]) == 0)
				break;

			msg++;
		}

		/* wait for "Press any key" */
		if (msg == MSG_DEBUG_ASK) {
			ctx->state = STATE_DEBUG_ASK;
			write(fd, "\r\n", 2);

		/* debug mode entered */
		} else if (msg == MSG_DEBUG) {
			/* if device supports it, switch to high baudrate */
			if ((ctx->flags & FLAG_SPEEDUP) && ctx->dev_setbaudrate != NULL) {
				ctx->state = STATE_SWITCH_BAUDRATE;
				write(fd, "ATBA5\r\n", 7);

			} else {
				ctx->state = STATE_XMODEM;

				if (ctx->flags & FLAG_CONFIG)
					write(fd, "ATLC\r\n", 6);

				else if (ctx->flags & FLAG_FIRMWARE)
					write(fd, "ATUR\r\n", 6);
			}

		/* follow device to high baudrate */
		} else if (msg == MSG_BAUD_HIGH) {
			ctx->dev_setbaudrate(ctx, B115200);
			linebuffer_clear(ctx->lbuf);

			ctx->state = STATE_XMODEM;

			if (ctx->flags & FLAG_CONFIG)
				write(fd, "ATLC\r\n", 6);

			else if (ctx->flags & FLAG_FIRMWARE)
				write(fd, "ATUR\r\n", 6);

		/* transfer was success */
		} else if (msg == MSG_XMODEM_OK && ctx->state == STATE_XMODEM_COMPLETE) {
			ctx->state = STATE_BOOT;
			write(fd, "ATGR\r\n", 6);

		/* follow device to low baudrate */
		} else if (msg == MSG_BAUD_LOW) {
			ctx->dev_setbaudrate(ctx, B9600);
			linebuffer_clear(ctx->lbuf);
		}

		free(line);
	};

	return 0;
}
