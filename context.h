#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "list.h"

struct context {
	struct list_head list;

	/* device local stuff */
	int fd;
	int (* dev_close)(struct context *context);
	int (* dev_setbaudrate)(struct context *context, int baudrate);
	char *devname;
	void *dev_privdata;

	/* event stuff */
	struct event_fd *event;

	/* statemachine */
	int state;

	/* line buffer */
	char linebuf[256];
	int linepos;

	/* xmodem */
	int lastpkt;
};

struct context * create_context(void);
int destroy_context(struct context *ctx);

int context_close(void);

#endif /* _CONTEXT_H_ */
