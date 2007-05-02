#ifndef _LINEBUFFER_H_
#define _LINEBUFFER_H_

struct linebuffer {
	int size;
	int pos;

	char data[0];
};

struct linebuffer * create_linebuffer(int size);

int linebuffer_read(struct linebuffer *lbuf, int fd);
char * linebuffer_get(struct linebuffer *lbuf);

int linebuffer_clear(struct linebuffer *lbuf);

#endif /* _LINEBUFFER_H_ */
