/***************************************************************************
 *   Copyright (C) 10/2006 by Olaf Rempel                                  *
 *   razzor@kopf-tisch.de                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
#include <sys/types.h>

#include "list.h"
#include "logging.h"

#include "event.h"

static LIST_HEAD(event_fd_list);
static LIST_HEAD(event_timeout_list);

struct event_fd {
	struct list_head list;
	unsigned int flags;
	int fd;
	int (*read_cb)(int fd, void *privdata);
	int (*write_cb)(int fd, void *privdata);
	void *read_priv;
	void *write_priv;
};

struct event_timeout {
	struct list_head list;
	unsigned int flags;
	struct timeval intervall;
	struct timeval nextrun;
	int (*callback)(void *privdata);
	void *privdata;
};

struct event_fd * event_add_fd(
			struct event_fd *entry,
			int fd,
			int type,
			int (*callback)(int fd, void *privdata),
			void *privdata)
{
	/* check valid filediskriptor */
	if (fd < 0 || fd > FD_SETSIZE) {
		log_print(LOG_ERROR, "event_add_fd(): invalid fd");
		return NULL;
	}

	/* check valid type (read/write) */
	if (!(type & FD_TYPES)) {
		log_print(LOG_ERROR, "event_add_fd(): invalid type");
		return NULL;
	}

	/* create new entry */
	if (entry == NULL) {
		entry = malloc(sizeof(struct event_fd));
		if (entry == NULL) {
			log_print(LOG_ERROR, "event_add_fd(): out of memory");
			return NULL;
		}

		memset(entry, 0, sizeof(struct event_fd));
		entry->flags |= EVENT_NEW;
		entry->fd = fd;

		/* put it on the list */
		list_add_tail(&entry->list, &event_fd_list);
	}

	if (type & FD_READ) {
		entry->flags = (callback != NULL) ? (entry->flags | FD_READ) : (entry->flags & ~FD_READ);
		entry->read_cb = callback;
		entry->read_priv = privdata;

	} else if (type & FD_WRITE) {
		entry->flags = (callback != NULL) ? (entry->flags | FD_WRITE) : (entry->flags & ~FD_WRITE);
		entry->write_cb = callback;
		entry->write_priv = privdata;
	}

	return entry;
}

int event_get_fd(struct event_fd *entry)
{
	return (entry != NULL) ? entry->fd: -1;
}

void event_remove_fd(struct event_fd *entry)
{
	/* mark the event as deleted -> remove in select() loop */
	entry->flags |= EVENT_DELETE;
}

static void add_timeval(struct timeval *ret, struct timeval *a, struct timeval *b)
{
	ret->tv_usec = a->tv_usec + b->tv_usec;
	ret->tv_sec = a->tv_sec + b->tv_sec;

	if (ret->tv_usec >= 1000000) {
		ret->tv_usec -= 1000000;
		ret->tv_sec++;
	}
}

static void sub_timeval(struct timeval *ret, struct timeval *a, struct timeval *b)
{
	ret->tv_usec = a->tv_usec - b->tv_usec;
	ret->tv_sec = a->tv_sec - b->tv_sec;

	if (ret->tv_usec < 0) {
		ret->tv_usec += 1000000;
		ret->tv_sec--;
	}
}

static int cmp_timeval(struct timeval *a, struct timeval *b)
{
	if (a->tv_sec > b->tv_sec)
		return -1;

	if (a->tv_sec < b->tv_sec)
		return 1;

	if (a->tv_usec > b->tv_usec)
		return -1;

	if (a->tv_usec < b->tv_usec)
		return 1;

	return 0;
}

static void schedule_nextrun(struct event_timeout *entry, struct timeval *now)
{
	add_timeval(&entry->nextrun, now, &entry->intervall);

	struct event_timeout *search;
	list_for_each_entry(search, &event_timeout_list, list) {
		if (search->nextrun.tv_sec > entry->nextrun.tv_sec) {
			list_add_tail(&entry->list, &search->list);
			return;

		} else if (search->nextrun.tv_sec == entry->nextrun.tv_sec &&
			   search->nextrun.tv_usec > entry->nextrun.tv_usec) {
				list_add_tail(&entry->list, &search->list);
				return;
		}
	}
	list_add_tail(&entry->list, &event_timeout_list);
}

struct event_timeout * event_add_timeout(
			struct timeval *timeout,
			int (*callback)(void *privdata),
			void *privdata)
{
	struct event_timeout *entry;
	entry = malloc(sizeof(struct event_timeout));
	if (entry == NULL) {
		log_print(LOG_ERROR, "event_add_timeout(): out of memory");
		return NULL;
	}

	entry->flags = 0;
	memcpy(&entry->intervall, timeout, sizeof(entry->intervall));
	entry->callback = callback;
	entry->privdata = privdata;

	struct timeval now;
	gettimeofday(&now, NULL);

	schedule_nextrun(entry, &now);
	return entry;
}

void event_remove_timeout(struct event_timeout *entry)
{
	/* mark the event as deleted -> remove in select() loop */
	entry->flags |= EVENT_DELETE;
}

int event_loop(void)
{
	fd_set *fdsets = malloc(sizeof(fd_set) * 2);
	if (fdsets == NULL) {
		log_print(LOG_ERROR, "event_loop(): out of memory");
		return -1;
	}

	while (1) {
		struct timeval timeout, *timeout_p = NULL;
		if (!list_empty(&event_timeout_list)) {
			struct timeval now;
			gettimeofday(&now, NULL);

			struct event_timeout *entry, *tmp;
			list_for_each_entry_safe(entry, tmp, &event_timeout_list, list) {
				if (entry->flags & EVENT_DELETE) {
					list_del(&entry->list);
					free(entry);
					continue;
				}

				/* timeout not elapsed, exit search (since list is sorted) */
				if (cmp_timeval(&entry->nextrun, &now) == -1)
					break;

				/* remove event from list */
				list_del(&entry->list);

				/* execute callback, when callback returns 0 -> schedule event again */
				if (entry->callback(entry->privdata)) {
					free(entry);

				} else {
					schedule_nextrun(entry, &now);
				}
			}

			if (!list_empty(&event_timeout_list)) {
				entry = list_entry(event_timeout_list.next, typeof(*entry), list);

				/* calc select() timeout */
				sub_timeval(&timeout, &entry->nextrun, &now);
				timeout_p = &timeout;
			}
		}

		fd_set *readfds = NULL, *writefds = NULL;
		struct event_fd *entry, *tmp;

		list_for_each_entry_safe(entry, tmp, &event_fd_list, list) {
			entry->flags &= ~EVENT_NEW;

			if (entry->flags & EVENT_DELETE) {
				list_del(&entry->list);
				free(entry);

			} else if (entry->flags & FD_READ) {
				if (readfds == NULL) {
					readfds = &fdsets[0];
					FD_ZERO(readfds);
				}
				FD_SET(entry->fd, readfds);

			} else if (entry->flags & FD_WRITE) {
				if (writefds == NULL) {
					writefds = &fdsets[1];
					FD_ZERO(writefds);
				}
				FD_SET(entry->fd, writefds);
			}
		}

		int i = select(FD_SETSIZE, readfds, writefds, NULL, timeout_p);
		if (i <= 0) {
			/* On error, -1 is returned, and errno is set
			 * appropriately; the sets and timeout become
			 * undefined, so do not rely on their contents
			 * after an error.
			 */
			continue;

		} else {
			list_for_each_entry(entry, &event_fd_list, list) {
				if ((entry->flags & EVENT_NEW) != 0) {
					/* entry has just been added, execute it next round */
					continue;
				}

				if ((entry->flags & FD_READ) && FD_ISSET(entry->fd, readfds))
					if (entry->read_cb(entry->fd, entry->read_priv) != 0)
						entry->flags |= EVENT_DELETE;

				if ((entry->flags & FD_WRITE) && FD_ISSET(entry->fd, writefds))
					if (entry->write_cb(entry->fd, entry->write_priv) != 0)
						entry->flags |= EVENT_DELETE;
			}
		}
	}
	free(fdsets);
}
