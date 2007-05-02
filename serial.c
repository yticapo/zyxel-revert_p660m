#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <termios.h>

#include "context.h"
#include "event.h"
#include "logging.h"
#include "statemachine.h"

struct serial_device {
	struct termios oldtio;
	struct termios newtio;
};

static int open_serial(struct context *ctx, const char *devname)
{
	struct serial_device *sdev = (struct serial_device *)ctx->dev_privdata;

	ctx->fd = open(devname, O_RDWR);
	if (ctx->fd < 0) {
		log_print(LOG_ERROR, "open_serial(): open(%s)", devname);
		return -1;
	}

	ctx->devname = strdup(devname);

	if (tcgetattr(ctx->fd, &sdev->oldtio) != 0) {
		log_print(LOG_WARN, "open_serial(): tcgetattr()");
		close(ctx->fd);
		return -1;
	}

	bzero(&sdev->newtio, sizeof(struct termios));
	sdev->newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD;

	tcflush(ctx->fd, TCIOFLUSH);

	if (tcsetattr(ctx->fd, TCSANOW, &sdev->newtio) != 0) {
		log_print(LOG_WARN, "open_serial(): tcsetattr()");
		close(ctx->fd);
		return -1;
	}

	return 0;
}

static int setbaudrate(struct context *ctx, int baudrate)
{
	struct serial_device *sdev = (struct serial_device *)ctx->dev_privdata;

	cfsetispeed(&sdev->newtio, baudrate);
	cfsetospeed(&sdev->newtio, baudrate);

	if (tcsetattr(ctx->fd, TCSANOW, &sdev->newtio) != 0) {
		log_print(LOG_WARN, "open_serial(): tcsetattr()");
		close(ctx->fd);
		return -1;
	}

	return 0;
}

static int close_serial(struct context *ctx)
{
	struct serial_device *sdev = (struct serial_device *)ctx->dev_privdata;

	event_remove_fd(ctx->event);

	tcsetattr(ctx->fd, TCSANOW, &sdev->oldtio);
	free(ctx->devname);
	close(ctx->fd);
	return 0;
}

int serial_init(struct context *ctx, const char *device)
{
	ctx->dev_privdata = malloc(sizeof(struct serial_device));
	if (ctx->dev_privdata == NULL) {
		log_print(LOG_WARN, "serial_init_cb(): out of memory");
		destroy_context(ctx);
		return -1;
	}

	if (open_serial(ctx, device) < 0) {
		free(ctx->dev_privdata);
		destroy_context(ctx);
		return -1;
	}

	log_print(LOG_EVERYTIME, "listen on %s", ctx->devname);

	ctx->dev_close = close_serial;
	ctx->dev_setbaudrate = setbaudrate;

	ctx->event = event_add_readfd(NULL, ctx->fd, statemachine_read, ctx);
	return 0;
}
