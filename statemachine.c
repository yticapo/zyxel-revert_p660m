#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <termios.h>

#include "context.h"
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

static int my_readline(int fd, struct context *ctx, char **retval)
{
	char *newline = memchr(ctx->linebuf, '\r', ctx->linepos);
	if (newline != NULL) {
		int len = newline - ctx->linebuf;
		*newline = '\0';
		if (len > 0)
			*retval = strdup(ctx->linebuf);

		ctx->linepos -= len +1;
		memmove(ctx->linebuf, newline +1, ctx->linepos);

		newline = memchr(ctx->linebuf, '\r', ctx->linepos);
		return ((*retval != NULL) ? 1 : 0) + ((newline != NULL) ? 1 : 0);
	}

	char buf[32];
	int len = read(fd, buf, sizeof(buf));
	if (len <= 0)
		return -1;

	int i, numlines = 0;
	for (i = 0; i < len; i++) {
		/* "understand" backspace */
		if (buf[i] == 0x08 && ctx->linepos > 0) {
			ctx->linepos--;

		/* export buffer */
		} else if (buf[i] == '\r') {
			if (*retval == NULL) {
				ctx->linebuf[ctx->linepos] = '\0';

				if (ctx->linepos > 0)
					*retval = strdup(ctx->linebuf);

				ctx->linepos = 0;
				numlines = 1;

			} else {
				ctx->linebuf[ctx->linepos++] = buf[i];
				numlines++;
			}

		/* copy */
		} else if (buf[i] >= ' ') {
			ctx->linebuf[ctx->linepos++] = buf[i];
		}
	}
	return numlines;
}

int statemachine_read(int fd, void *privdata)
{
	struct context *ctx = (struct context *)privdata;

	if (ctx->state == STATE_XMODEM) {
		if (xmodem_read(fd, privdata) < 0)
			ctx->state = STATE_XMODEM_COMPLETE;

		return 0;
	}

	int numlines;
	do {
		char *line = NULL;
		numlines = my_readline(fd, ctx, &line);
		if (numlines < 0)
			return -1;

		if (line == NULL)
			return 0;

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
			if (ctx->dev_setbaudrate != NULL) {
				ctx->state = STATE_SWITCH_BAUDRATE;
				write(fd, "ATBA5\r\n", 7);

			} else {
				ctx->state = STATE_XMODEM;
				write(fd, "ATLC\r\n", 6);
			}

		/* follow device to high baudrate */
		} else if (msg == MSG_BAUD_HIGH) {
			ctx->dev_setbaudrate(ctx, B115200);
			numlines = 0;

			ctx->state = STATE_XMODEM;
			write(fd, "ATLC\r\n", 6);

		/* transfer was success */
		} else if (msg == MSG_XMODEM_OK && ctx->state == STATE_XMODEM_COMPLETE) {
			ctx->state = STATE_BOOT;
			write(fd, "ATGR\r\n", 6);

		/* follow device to low baudrate */
		} else if (msg == MSG_BAUD_LOW) {
			ctx->dev_setbaudrate(ctx, B9600);
			numlines = 0;
		}

		free(line);
	} while (numlines > 1);

	return 0;
}
