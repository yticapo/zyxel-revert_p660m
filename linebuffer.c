#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "linebuffer.h"

struct linebuffer * create_linebuffer(int size)
{
	struct linebuffer *retval = malloc(sizeof(struct linebuffer) + size);
	if (retval == NULL)
		return NULL;

	retval->size = size;
	retval->pos = 0;
	return retval;
}

int linebuffer_read(struct linebuffer *lbuf, int fd)
{
	char buf[32];
	int len = read(fd, buf, sizeof(buf));
	if (len <= 0)
		return -1;

	int i;
	for (i = 0; i < len; i++) {
		/* "understand" backspace */
		if (buf[i] == 0x08 && lbuf->pos > 0) {
			lbuf->pos--;

		/* copy */
		} else if (buf[i] >= ' ' || buf[i] == '\r') {
			lbuf->data[lbuf->pos++] = buf[i];
		}

		if (lbuf->pos > lbuf->size)
			return -1;
	}

	return 0;
}

char * linebuffer_get(struct linebuffer *lbuf)
{
	char *newline = memchr(lbuf->data, '\r', lbuf->pos);
	if (newline == NULL)
		return NULL;

	char *retval = NULL;
	*newline = '\0';

	int len = newline - lbuf->data;
	if (len > 0)
		retval = strdup(lbuf->data);

	lbuf->pos -= len +1;
	memmove(lbuf->data, newline +1, lbuf->pos);

	return retval;
}

int linebuffer_clear(struct linebuffer *lbuf)
{
	int oldpos = lbuf->pos;
	lbuf->pos = 0;
	return oldpos;
}
