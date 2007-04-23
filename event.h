#ifndef _EVENT_H_
#define _EVENT_H_

#include <sys/time.h>

#define EVENT_NEW	0x1000
#define EVENT_DELETE	0x2000

#define FD_READ		0x0001
#define FD_WRITE	0x0002
#define FD_TYPES	(FD_READ | FD_WRITE)

#define event_add_readfd(entry, fd, callback, privdata) \
	event_add_fd(entry, fd, FD_READ, callback, privdata)

#define event_add_writefd(entry, fd, callback, privdata) \
	event_add_fd(entry, fd, FD_WRITE, callback, privdata)

/* inner details are not visible to external users (TODO: size unknown) */
struct event_fd;
struct event_timeout;

struct event_fd * event_add_fd(
			struct event_fd *entry,
			int fd,
			int type,
			int (*callback)(int fd, void *privdata),
			void *privdata);

int event_get_fd(struct event_fd *entry);
void event_remove_fd(struct event_fd *entry);

struct event_timeout * event_add_timeout(
			struct timeval *timeout,
			int (*callback)(void *privdata),
			void *privdata);

void event_remove_timeout(struct event_timeout *entry);

int event_loop(void);

#endif /* _EVENT_H_ */
