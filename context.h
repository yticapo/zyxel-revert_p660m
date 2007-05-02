#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "filedata.h"
#include "linebuffer.h"
#include "list.h"

#define FLAG_CONFIG 0x01
#define FLAG_FIRMWARE 0x02
#define FLAG_SPEEDUP 0x04

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
	int flags;

	/* line buffer */
	struct linebuffer *lbuf;

	/* xmodem */
	int lastpkt;

	struct filedata *file;
};

struct context * create_context(const char *filename);
int destroy_context(struct context *ctx);

int context_close(void);

#endif /* _CONTEXT_H_ */
